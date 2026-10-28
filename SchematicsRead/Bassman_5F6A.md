# Fender Bassman 5F6-A (tweed, 4x10) - schematic read

- Source file: `4e57f754-Fender_Bassman_5F6a.pdf` (1 page, scan, raster 150 dpi -> 2403x1396, zoomed crops used)
- Title printed: "FENDER 'BASSMAN' SCHEMATIC  MODEL 5F6-A"; small mark "I-EG" (initials?) under the title. No date or revision letter printed.
- Notice printed: "VOLTAGES READ TO GROUND WITH ELECTRONIC VOLTMETER. VALUES SHOWN +/- 20%"
- Parts-box printed: POWER TRANS - 8087, CHOKE - 14684, OUTPUT TRANS - 45249.
- Units: capacitors printed as "value-voltage" (e.g. ".02-400" = 0.02 uF / 400 V). ".0001", ".00025" are uF (100 pF, 250 pF). "47 MMF" = 47 pF. Electrolytics "8-450", "20-600 P" etc. in uF-V.

## 1. Tube complement / heaters
| Pos | Tube | Function |
|---|---|---|
| V1 | 12AY7 (both halves) | One half per channel: BRIGHT and NORMAL |
| V2 | 12AX7 | V2a gain stage (mixer/recovery), V2b cathode follower (direct-coupled) |
| V3 | 12AX7 | Long-tailed phase inverter |
| V4, V5 | 5881 x2 | Push-pull output (cathode grounded, fixed bias) |
| Rect | GZ34 | Full-wave tube rectifier |

Heaters: "TO ALL 6.3 VOLT HEATERS AND PILOT LIGHT" from the power transformer (3 leads at bottom of PT, one grounded center-tap arrow). No heater voltages/wiring drawn for individual tubes. AC input 117 V AC, 3 AMP FUSE, AC SWITCH, GROUND SWITCH (3-pos shown, with .05-600 cap from the ground-switch contact to chassis ground), STANDBY SWITCH (see supply).

## 2. Preamp stages
### Input networks (each channel, identical)
- Two jacks per channel, "1" and "2". Jack 1 node: **68k** in series to the V1 grid node; Jack 2 node: **68k** to the same grid node (i.e. the grid sits between the two 68k resistors). Jack-2 switch contact is wired to the jack-1 node (normalling). **1M** from the jack-1 node to ground (drawn diagonally to ground). No grid-stop resistor besides the 68k's. No separate grid-leak value shown on the grid itself (the 1M shunt acts as the leak).

### V1a - BRIGHT channel (12AY7 half)
- Plate load: **100k** to the +325 V node (the two V1 plate loads are 100k + 100k joined at a node that goes to the +325 V supply node, see section 7). Plate voltage printed **+150 V**.
- Cathode: **820 ohm** in parallel with **250 uF / 6 V**, shared by both V1 halves; cathode voltage printed **+2.5 V**.
- Coupling: plate -> **.02 uF/400 V** -> top of VOLUME pot. VOL pot **1M** (taper not printed). Bright cap: **.0001 uF (100 pF)** connected from the volume pot top (after the .02) to the pot wiper, i.e. across the upper part of the pot.
- Wiper -> **270k** mixing resistor -> mix node (V2a grid).

### V1b - NORMAL channel (12AY7 half)
- Identical: plate load 100k, shared cathode 820 // 250 uF/6 V, plate +150 V, .02/400 coupling to VOL pot **1M**. No bright cap. Wiper -> **270k** -> same mix node.

### Mixer / V2a (12AX7 first half)
- Grid = the junction of the two 270k mixing resistors (no other grid resistor, no grid-stop drawn).
- Cathode: **820 ohm to ground, NOT bypassed** (no cap drawn); cathode voltage **+1.2 V**.
- Plate load: **100k** to the +325 V node; plate voltage **+180 V**.
- Plate is **direct-coupled** to V2b's grid (no coupling cap).

### V2b - cathode follower (12AX7 second half)
- Plate tied directly to the **+325 V** node (printed +325 V).
- Cathode resistor: **100k** to ground. Cathode voltage printed **+180 V**.
- Cathode output feeds the tone stack directly (no coupling cap before the stack).

## 3. Tone stack ("tweed Bassman" stack)
Fed from V2b cathode; output taken at the TREBLE pot wiper, then through .02 uF into the phase inverter.
- **56k** resistor between node A (stack input) and node B.
- **.00025 uF (250 pF)** from node A to the top of the TREBLE pot.
- Node B: **.02/400** to the bottom of the TREBLE pot (= top of the BASS rheostat); node B also continues down to a third **.02/400** whose other side goes to the **wiper** of the MIDDLE pot.
- TREBLE pot: **250k** (taper not printed). Wiper = stack output.
- BASS pot: **1M**, drawn as a variable resistor (rheostat: wiper tied to one end) in series between the bottom of the treble pot and the top of the middle pot.
- MIDDLE pot: **25k**, from the bass pot bottom to ground; its wiper receives the third .02 uF.
- Output: treble wiper -> **.02-400** -> PI upper grid node (see 5).
- Presence: see section 6. No bright/depth switches.
- Pot tapers (audio/linear) are not printed - not legible/not given.

## 4. Reverb / tremolo / vibrato
None.

## 5. Phase inverter (V3, 12AX7, long-tailed pair)
- Upper (input) grid fed from the tone stack via **.02-400**; voltage printed **+22 V** at that grid.
- Lower grid: its own **.1-200** cap goes to the NFB/presence line (signal return, see below); grid voltage printed **+23 V**.
- Grid resistors: **1M** from upper grid to a bias junction node, **1M** from lower grid to the same node (junction printed **+32.5 V**).
- That junction connects via **10k** to the NFB line (the vertical line carrying the 27k feedback resistor, the 5k presence pot to ground). The cathode node (printed **+54 V**) is joined to the 1M junction through **470 ohm**. So the DC path to ground is: cathodes -> 470 -> junction -> 10k -> NFB line -> 5k presence pot -> ground (about 15.5k total). (Read as drawn; the cathode "tail" is therefore NOT a single resistor to ground.)
- Both cathodes tied together (+54 V).
- Plate loads: upper plate **82k, 5%**; lower plate **100k, 5%**. The two loads join at a center node fed from the **+385 V** node (vertical line down to the 385 V node). Plate voltages printed: upper **+235 V**, lower **+230 V**.
- **47 MMF (47 pF)** capacitor connected between the two plates.
- Output coupling: **.1-400** from each plate to each output-tube grid.

## 6. Output stage
- Tubes: 2x **5881**, class AB push-pull, **fixed bias**, cathodes grounded directly (cathodes drawn to a ground arrow, no resistor).
- Grid resistors: each coupling cap -> grid with **220k** from each grid to a common bias node (two 220k in series between the grids; center node = bias line). Bias voltage printed **-48 V**.
- Screen resistors: **470 ohm, 1 W** each, from each screen to the "+430 V" node (the node before the choke).
- Plates: to OT primary ends; OT primary center tap to **+432 V** (after the choke).
- Printed voltages: plates/center tap +432 V, screens/430 node **+430 V**, bias **-48 V**. Individual tube plate/screen voltages are not printed separately.
- Bias supply: tap from the power transformer (a separate lead from the HV winding region, line going up from the transformer to a diode, left side, top of the supply) -> **diode** (semiconductor symbol) -> **8 uF/150 V** electrolytic (marked 8-150, polarity as drawn: + toward the ground arrow) -> **15k** series -> **-48 V** node, with **8 uF/150 V** (8-150) and **56k** to ground at that node. No bias pot is shown (no adjustment drawn).
- Output transformer (45249): primary center tapped; secondary drawn with a single winding (two ends, one end grounded, the other the "hot" side) feeding **four parallel speaker jacks** (4 x 10" speakers). No impedance/ratio numbers printed. Speaker load/impedance not printed.
- Global negative feedback: **27k** from the hot (top) side of the OT secondary, via a wire running across the top of the drawing, down to the NFB line, which connects to the **PRES. 5k** pot top (with **.1-200** cap from the pot wiper to ground end... drawn as the cap between pot wiper and the pot bottom/ground) and via **10k** to the PI 1M-junction / cathode circuit (see 5). Presence control: **5k** pot (taper not printed) in the feedback return with **.1 uF/200 V** cap across the lower part (wiper to ground). The lower PI grid also sees this line through its .1-200 cap.

## 7. Power supply
- Mains 117 V AC, 3 AMP FUSE, AC switch, ground switch with **.05-600** cap, standby switch (with **.05-600** cap from the HV line before the switch to ground at the switch pole).
- Rectifier: **GZ34**, anode windings printed "**325 V. AC**" per side.
- After the standby switch: reservoir node **+432 V** with **20-600 P** x2 (two 20 uF/600 V caps to ground, "P" probably = polarized), choke (**14684**) from +432 V node to the **+430 V** node (**20-600 P** cap), then **4700 ohm** to the **+385 V** node (**20-600 P** cap), then **10k** to the **+325 V** node (**8-450** cap).
  - Note: the choke is shown between the +432 V node (rectifier side, with the two 20 uF caps) and the +430 V node (with one 20 uF cap); the OT center tap goes to the rectifier-side/+432 V line and the screens to the +430 V node. (Printed labels: "+430 V" on the screen/choke-input line, "+432 V" on the OT center-tap line.)
  - +385 V node feeds the PI plate loads (82k/100k).
  - +325 V node feeds V1 plate loads (100k x2), V2a plate load (100k), V2b plate.
- Other printed voltages: V1 plates +150 V; V1 cathode +2.5 V; V2a cathode +1.2 V; V2a plate +180 V; V2b cathode +180 V; PI grids +22/+23 V; PI cathodes +54 V; PI plates +235/+230 V; bias -48 V.

## 8. Not legible / uncertain
- Exact node where the rectifier's "+432 V" is taken relative to the choke (which side the screens and plates connect) - read as plates = +432 V line, screens = +430 V line (labels clear, wiring somewhat ambiguous).
- Polarity of the bias supply diode and electrolytics (symbols small): read as negative supply of -48 V.
- Pot tapers not printed. OT impedance, turns ratio and speaker impedances not printed.
- Presence cap connection ("wiper to ground") (?).
- "I-EG" mark meaning unknown. No date/revision.
- Whether the 1M input shunt belongs only to jack 1 (as drawn) (?).
