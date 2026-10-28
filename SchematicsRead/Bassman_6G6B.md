# Fender Bassman 6G6-B (blonde, 2-channel Bass/Normal) - schematic read

- Source file: `bbee01f0-Fender_Bassman_6G6b.pdf` (1 page A4-ish scan, rasterized 150 dpi -> 2371x1213; text is low resolution/pixelated, many labels are rotated and hard to read; zoomed crops were used)
- Printed title: "FENDER 'BASSMAN' SCHEMATIC  MODEL 6G6-B"; marks "P/O" and "E-FB" (initials?) near title. Footer: "FENDER ELECTRIC INSTRUMENT CO.- FULLERTON, CAL." No date/revision beyond the "-B" in the model code.
- Printed NOTES: (1) Voltages read to ground with electronic voltmeter, values shown +/-20%. (2) All resistors 1/2 watt, 10% tolerance if not specified. (3) All capacitors at least 400 volt rating if not specified.
- Parts box printed: TR1 - 125P7A (power transformer), TR2 - 125C1A (choke), TR3 - 125A13A (output transformer).
- Cap notation: ".0005", ".25", ".1" are uF; "20-600 P" = 20 uF / 600 V; "25-50" = 25 uF / 50 V; ".1-200" = 0.1 uF / 200 V.

## 1. Tube complement / heaters
| Pos | Tube | Function |
|---|---|---|
| Bass V1a / V1b | 7025 (12AX7 low-noise equivalent) | V1a gain stage, V1b cathode follower (direct-coupled) |
| Bass V2a / V2b | 7025 | V2a gain stage after stack/volume, V2b gain stage (treble pot in between) |
| Normal V1 / V2 | 7025 | V1 gain stage -> passive stack -> volume -> V2 gain stage |
| PI | 7025 | Long-tailed phase inverter |
| Output (2) | 5881 | Push-pull, fixed bias, cathodes grounded |
| Rectifier | Silicon diode stacks (3 diodes in series per leg, 2 legs, center-tapped HV) | No rectifier tube |

(Four "7025" labels are printed: bass V1a/V1b, bass V2a/V2b, normal V1+V2 (one label over two triode symbols), and the PI. Exact envelope-to-triode grouping as drawn; every stage is a 12AX7-type triode.)
Heaters: "TO ALL HEATERS AND PILOT LIGHT" - three leads from TR1 with a grounded center tap; AC 117 V (plug), AC SWITCH, 3 AMP FUSE, GROUND SWITCH with ".05 / 600" cap.

## 2. Preamp stages
### Input networks (both channels same)
- Two jacks per channel ("1","2"). 68k from each jack node to the grid node (grid between two 68k); 1M from the jack-1 node to ground. Jack-2 switch contact tied to the jack-1 node.

### BASS channel
**V1a (7025 half)**: grid = junction of the two 68k. Cathode 2700 ohm || 25 uF (cap value label reads "25" / "25" (?), voltage not readable), cathode +1.1 V. Plate load **330k (?)** (label reads like "330K"; rotated/blurred) to the supply line; plate voltage **+135 V**.
**V1b (7025 half)**: grid direct from V1a plate (direct coupled). Plate tied directly to the supply line (label **+350 V (?)**, could be +250 V). Cathode 100k to ground; cathode voltage **+134 V** (cathode follower). Output from cathode.

**Bass-channel tone network (between V1b cathode and volume, as drawn)**:
- V1b cathode node N1. From N1: **.00025 uF** to the top of a **47k** resistor, 47k down to node J (J = top of VOLUME pot). N1 also -> **1M** -> node N2.
- N2 -> **.25 uF** -> node P; N2 -> (down) -> **.25 uF** -> node Q.
- J -> **10k** -> node P. BASS pot **250k (taper label reads "250K-L" or "250K-A", (?))** drawn as a rheostat between P and Q. Q -> **820 ohm** -> ground.
- J -> VOLUME pot **250k (label "250K-L" (?))**, bottom to ground, wiper to V2a grid.

**V2a (7025 half)**: grid = volume wiper. Cathode **1500 ohm || 25 uF (?)**, cathode +1.2 V. Plate load **100k** to the +290 V (?) node; plate voltage **+150 V**.
- Coupling: plate -> **.1 uF** -> TREBLE pot **250k-A** top; treble pot bottom -> (**.01 uF** in parallel with **1M**) -> ground; treble wiper -> V2b grid.

**V2b (7025 half)**: cathode **1500 ohm to ground, unbypassed** (+1.3 V). Plate load: **100k in parallel with a small cap (label unreadable, ~.002 or .005 (?))**; plate voltage **+150 V**. The load's supply end is the node that reaches B+ through the 56k/1W decoupling. Plate output -> **470k** series resistor -> shared mixing node (see below).

### NORMAL channel
**V1 (7025 half)**: grid between the two 68k, 1M shunt on jack 1. Cathode **1500 ohm || 25 uF (?)**, cathode +1.9 V. Plate load **100k** to the +365 V line (line label "+365 V"); plate voltage **+230 V**.
- Tone network on the plate (passive stack, no extra gain stage): plate node P1 -> **100k** series -> node S; P1 -> **.00025 uF** -> top of TREBLE pot. S -> **.1 uF** -> node T (T joined to the treble pot lower/tap terminal). S -> **.05 uF** -> bass pot bottom node U; BASS pot **250k-A (?)** rheostat between T and U; U -> **6800 (?)** (label reads "6800"/"6800") -> ground. TREBLE pot **250k (taper A?)** has a tap/lower point connected to T; its bottom end goes through a **.001 uF (?)** (label unclear, "TONE TRAP"/"TAP" text near it, not legible) to ground. Treble wiper -> VOLUME pot **1M-A** top; volume bottom to ground; volume wiper -> V2 grid.
**V2 (7025 half, normal)**: cathode **2700 ohm || 25 uF (?)**, cathode +1.8 V. Plate load **220k** to the +355 V line; plate voltage **+190 V**. Plate -> **470k** series -> shared mixing node.

### Channel mixing
- Bass V2b plate and Normal V2 plate each go through **470k** to a common node; from this node a **.0005 uF (?)** cap goes to the PI upper grid.

## 3. Tone stack
See above: Bass channel has the 47k/.00025/1M/.25/.25/10k/820 network with bass rheostat then volume (250k) - no treble pot there; its treble control is the **250k-A** pot after V2a with .01 || 1M to ground. Normal channel has 100k / .00025 / .1 / .05 / 6800 (?) / treble 250k with tap / bass 250k. No presence in the stack; presence is in the NFB loop (section 6). No bright, depth or mid pots.

## 4. Reverb / tremolo / vibrato
None.

## 5. Phase inverter (7025 long-tailed pair)
- Input: from the mixing node via **.0005 uF (?)** to the upper grid.
- Grid resistors: **1M** from each grid to a common junction (junction printed approx **+18 V (?)** by dotted arrow; upper grid and lower grid values not separately printed).
- Junction -> **6800 (?)** -> NFB/presence line; junction -> **820 ohm** -> the shared cathode node (printed **+30 V**).
- Lower grid: **.1 uF / 200 V** cap to the NFB line (signal return).
- Plate loads: upper **82k 5% (?)**, lower **100k 5%**; centre node of the two loads is fed from the **+410 V** node. Plate voltages: upper **+300 V**, lower **+280 V**.
- A cap of **.0001 uF (100 pF) (?)** connects the two plates (label hard to read).
- Output coupling: **.1 uF** (voltage not printed; note says >=400 V) from each plate to its output grid.

## 6. Output stage
- 2 x **5881** push-pull, cathodes grounded directly (no resistor), **fixed bias -54 V**.
- Grid resistors: **220k 5%** per grid (value pixelated but reads 220K), to a bias bus at **-54 V**.
- Screen resistors: **470 ohm, 1 W** each, from each screen to the "+428 V" node (after the choke).
- Printed plate voltage at each tube **+428 V**. OT primary center tap labeled **+450 V** (taken before the choke at the rectifier output). Screen supply line labeled "+428 V".
- Output transformer TR3 (125A13A). Secondary shown as a single winding (one end grounded) feeding two speaker jacks with speaker symbols; no impedance/ratio printed.
- Bias supply: separate tap from TR1 -> **1k (resistor, 1W? label "1K 1W-5%"? (?))** -> diode -> **-54 V** bus; **25 uF / 50 V** electrolytic and **27k 1W 5%** from the bus to ground. No bias adjustment pot.
- Global NFB: **56k** from the hot (top) side of the OT secondary (top of drawing, long wire) down to the NFB line. The line goes to ground through a **4700 ohm** resistor; in parallel, a **PRESENCE 25k** pot (rheostat) in series with **.1 uF / 200 V** to ground. The same line feeds the PI lower grid (via .1-200) and the PI grid-junction via 6800 (?).

## 7. Power supply (solid state)
- TR1 high-voltage winding center tap grounded; each end through **3 silicon diodes in series** to the rectifier output node; printed **+450 V**.
- Output node: **2 x 20 uF / 600 V** (20-600 P) caps to ground; then STANDBY SWITCH; then choke **TR2 (125C1A)**; then node **B** (printed +428 V (?) at plates/screens) with **20-600 P** to ground.
- From node B: **4700 ohm 1 W** -> **+410 V** node (**20-600 P**) -> PI plate loads. From the +410 V node: **27k 1 W** -> **+355 V** node (**20-600 P**), marked "C" (feeds normal channel V2 plate (220k) and V1 plate (100k), labeled +365/+355 V).
- From node B (terminal "B" at left): **56k 1 W** -> bass-channel node (**20-600 P**) -> "+290 V (?)" line (bass V2a/V2b loads) and the bass V1 line (+350/+250 V (?)).
- Heater/pilot: 6.3 V (not printed) with centre tap grounded.
- Mains: 117 V AC, AC switch, 3 amp fuse, ground switch with .05/600 cap.

## 8. Not legible / uncertain
- Bass V1a plate load 330k (?), bass V1 supply line voltage (+350 V or +250 V) (?).
- Cathode bypass cap values/voltages (all read "25", voltage not readable).
- Bass V2b plate shunt cap value; PI plate-to-plate cap (.0001?); PI input cap (.0005?); resistor between PI grid junction and NFB line (6800 vs 4700?).
- Normal-channel treble pot taper/tap labeling ("TONE TRAP"?) and the cap to ground from its bottom.
- Pot tapers for bass-channel pots (L vs A).
- PI plate load 82k (?).
- Bias supply series resistor value/wattage, diode direction (read as negative supply).
- OT impedance/turns ratio, speaker impedance not printed.
- Heater voltages not printed.
