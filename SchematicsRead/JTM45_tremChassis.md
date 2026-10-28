# Marshall JTM45 tremolo chassis (1961 / 1962 / 1987T) - schematic read

- Source file: `jtm45tr.gif` (1 page, 2194x1539, 1-bit style scan, hand-lettered; many small numerals blobbed; zoomed crops used)
- Title printed (top left): "JTM 45  BASIC SCHEMATIC FOR MARSHALL TREM AMPS  TYPES 1961, 1962, 1987/T"
- No date, no revision letter, no part-number box printed.
- Notes printed bottom left: "ALL RESISTORS ARE 1/4 WATT UNLESS OTHERWISE STATED" / "ALL VOLTAGES MEASURED TO CHASSIS UNDER NO SIGNAL CONDITIONS WITH AN AVO MODEL 8 MK II METER SENSITIVITY 20,000 ohm/V".
- Note printed bottom right (under the rectifier): "IF V7 IS NOT FITTED THEN RECTIFICATION IS BY IN4007 OR SIMILAR DIODES AND OUTPUT VALVES ARE EL34" (so the drawn output valves, KT66, are the rectifier-valve version; EL34 + diodes is the alternative).
- Mains block printed: "ac. Mains 110V-250V 50/60 cps", "2A SLOW BLOW" (mains fuse), "500mA SLOW BLOW" (HT fuse on the HT winding centre tap).
- Units: capacitors printed without units: ".02", ".01", ".1", "0.1", ".022" = uF; "270pF", "100pF", "56pF" are labelled pF; electrolytics "250" (V1 cathode), "50" (trem cathode) = uF with +/- marks; resistors "K" = kohm, "M" ("1M", "2M") = Mohm.
- Voltages: circled values on the schematic: **310V**, **380V**, **440V** (read "4 4 0V", the first digit is smudged; also the chart says 440), **450V**, **105V** (bias area, sign not drawn, see 6.).

## 1. Tube complement
| Pos | Tube printed | Function |
|---|---|---|
| V1 | ECC83 | V1A (left half) and V1B (right half): the two input channels (one half per channel) |
| V2 | ECC83 | Tremolo oscillator (one triode used) |
| V3 | ECC83 | V3A voltage-gain stage (mixer/recovery), V3B cathode follower (direct-coupled) |
| V4 | (ECC83, labelled "V4A"/"V4B") | Long-tailed phase inverter |
| V5, V6 | KT66 (printed "KT 66") | Push-pull output |
| V7 | rectifier, label reads "GZ34" (? letters blobby, "6Z3a"-like) | Full-wave tube rectifier |
| Q1 | 2G37x (printed "2G 37A"? the last character is blobby) (?) | Tremolo output transistor (PNP/NPN not resolved) |

Heaters: the voltage chart marks the ECC83 heaters at pin 9 and the octal tubes at pins 2 and 7 as "6.3V AC HEATER" in abbreviated form ("3V AC HEATER", first digit hidden by smudging) (?); V7 heater wiring drawn as two separate heater leads to the rectifier cathode.

## 2. Preamp (V1, two channels)
### Input networks (4 jacks, 2 per channel; each channel drawn identically)
- Jack A: **68K** in series from jack A's contact to the V1 grid node. Jack B: **68K** from jack B's contact to the same grid node. Jack-B normalling contact is wired to jack A's contact. **1M** from the jack-A/jack-B contact node to ground (drawn at the foot of each jack pair). So each grid has two 68K in a "wired-together" input like the Fender, with a 1M shunt.
- Top pair feeds V1A grid (left half) and bottom pair feeds V1B grid (right half). V1A and V1B share one cathode.

### Cathode
- V1 (both halves): **820 ohm** in parallel with **250 uF** (electrolytic, + toward the cathode); shared by A and B.
- Chart voltages for V1: pin 3 and pin 8 (cathodes) **1.6 V**.

### Plate loads and coupling
- V1A plate load: **180K** (read "180K", the 1 is clear in the enlarged crop). Plate voltage (pin 1) **220 V**.
- V1B plate load: **100K**. Plate voltage (pin 6) **220 V** (chart prints 220 for both).
- V1A plate -> **.02** coupling -> top of a **1M** volume pot (no bright cap on this one).
- V1B plate -> **.02** coupling -> top of a **1M** volume pot with **100pF** bright cap from the pot top to the wiper (the cap is drawn from the junction after the .02 to the pot wiper side). The 1M on this side is drawn as a vertical pot with a 100pF cap across the upper part.
- Pot tapers not printed.

### Mix / V3A
- Each volume pot wiper goes to a **270K** mixing resistor; **56pF** (read "56pf", with a stray "5" in front; may be "5.6pF"?) is drawn in parallel with the right-hand 270K (the V1B/bright side). Both 270K go to the common mix node which goes up to V3A grid. (Drawing is crowded at this point: the mix node line crosses the ground line with a dot; read as drawn, the dot may be a drawing artefact (?).)
- V3A cathode: **820 ohm** to ground, not bypassed (no cap drawn). Chart cathode (pin 3) **1.1 V**.
- V3A plate load: **100K** (printed "100K"), plate (pin 1) **190 V**.
- V3A plate is direct-coupled to V3B grid (chart: pin 7 grid at 190 V).

### V3B cathode follower
- Plate tied to the **310 V** node (chart pin 6 = **310 V**).
- Cathode resistor **100K** to ground; cathode (pin 8) **190 V** (chart).
- Cathode output feeds the tone stack directly.
- The V3 plate loads and V3B plate hang from the circled **310V** node (that node has a **16 uF** electrolytic to ground and is fed through **10K 1W** from the **380V** node).

## 3. Tone stack (Fender-Bassman-style layout)
Input from V3B cathode; output taken at the treble pot wiper.
- **270pF** from stack input node to the top of the TREBLE pot.
- **56K** from stack input node down to node B.
- **.02** from node B across to the bottom of the TREBLE pot (top of the BASS pot).
- TREBLE pot **250K** (printed "250K"), wiper = output.
- BASS pot **1M**, drawn as a rheostat (wiper tied to the pot's upper end, wire drawn round the left side).
- MIDDLE pot **25K**, from the bass pot bottom to ground; its wiper receives a **.02** whose other side goes back to node B (the 56K bottom node).
- Output: treble wiper -> **.02** -> V4 upper grid.
- No bright switch, no volume pot after the stack, no master.

## 4. Tremolo (V2 + 2G37x)
Read from the lower-left block. Values:
- V2 (ECC83): plate from a **330K** that goes to "TO JUNCTION OF 1K, 8.2K AND 20H CHOKE" (printed: "1K, 82K" in the enlarged text reads "8.2K") (the HT point after the reservoir; this is where the 1K 2W screen supply, the 8.2K 1W and the choke meet).
- **.022** from V2 plate to the left end of the network (feedback to the phase-shift chain), **.01** and **.01** coupling caps in series along the grid line, **1M** from the first node to V2 cathode, **1M** from the grid node to ground, **2M** potentiometer (speed? function not labelled) with a second pot/resistor below it printed "OOK" (read as 100K (?)) drawn as a mechanically ganged pair on the left edge.
- V2 cathode: **27K** (printed "27K"?) in parallel with **50 uF** (+ toward the cathode) to ground.
- **150K** from the V2 plate/HT node to ground (drawn as a vertical resistor beside V2) (?role: bleed/divider).
- Output side: **680K** then **0.1** coupling to the base of the transistor (2G37x); **8.3K** (read "8.3K" / "i3K", first glyph blobby) (?) from the base node to ground. Transistor emitter/collector to ground and via a foot-switch contact ("FOOT SWITCH") to the wiper of a **50K** pot (tremolo depth). Chain from the mix node: **1 uF?** (cap drawn with the value glyph merged: "1" visible) -> **10K** -> **50K** pot -> **33K** -> ground. The tremolo signal enters the signal path at the mix node (the line from the 270K junction).
- Voltages: V2 chart pin 1 **140 V**, pin 3 **1.2 V**, pin 2 **0 V**. No other tremolo voltages printed.

## 5. Phase inverter (V4, long-tailed pair; same topology as the Bassman 5F6-A)
- Upper grid fed from the tone stack via **.02**; **1M** from the upper grid to the bias junction node; **1M** from the lower grid to the same node.
- Lower grid: its own **0.1** cap goes to the NFB/presence line.
- Junction node: **10K** to the NFB line (the vertical carrying the 27K feedback resistor and 5K presence pot) and **470 ohm** from the junction node to the (joined) PI cathodes. Chart: PI cathodes (pin 3, pin 8) **40 V**; grids "+" (printed "+" in the chart for pin 2 and 7).
- Plate loads: **82K** (V4A, printed "82K"/"92K" blobbed, read as 82K as it is paired with 100K) and **100K** (V4B). Both go to the **380V** node. Plate voltages (pin 1, pin 6) **250 V** each.
- A capacitor is drawn between the two plates; its label is blobbed (read "pF" with a number like "47"? ) (?). Not read confidently.
- Output coupling: **0.1** from each plate to each output grid.

## 6. Output stage
- Tubes: 2x **KT66** (printed "KT 66"; EL34 with silicon rectifier in place of V7 if not fitted, per the note).
- Grid circuit: each coupling cap (0.1) -> a grid; two resistors in series between the grids with their centre node to the bias line. The resistor labels are blobbed and look like "22K"/"33K" (two numerals + K) (?). Not read confidently (the Bassman uses 220k; this could be a 3-digit label squeezed to look 2-digit).
- Screens: each screen has a **470 ohm 1W** (printed "470 1W", two of them, one per tube) to a common screen node; that node goes via **1K 2W** (printed "1K 2W") to the HT after the choke (the +440 V line). Chart pin 4 (screen) = **440 V** on both V5 and V6.
- Plates: to OT primary ends; centre tap to the **440V** line after the choke; chart pin 3 (plate) = **430 V** on both V5, V6.
- Cathodes: tied together and returned via the bias capacitor/bias network, not directly to ground. A capacitor (printed "I", value (?)) sits between the common cathode and ground (drawn as a non-polarised cap), i.e. fixed bias with the cathodes at chassis. Chart pin 8 cathodes not printed.
- Bias supply: HT winding tap at the rectifier anode side -> **diode** -> **150K** -> bias reservoir electrolytic (+ / - marks, value blobbed "5" or "8" uF (?)), and a divider from the bias line: **56K** (printed "5 6K") to the output grid node and **15K** ("16K"/"15K", read "15K"?) to the common cathode capacitor. The circled **105V** is shown with an arrow at the bias-line node above the 56K (voltage as printed; sign and exact measuring point not drawn) (?).
- No bias pot is drawn.
- Output transformer: primary centre-tapped. Secondary: taps labelled in blobbed handwriting; the top one reads "WHITE 100V" (?; first word legible as WHITE, "100V" probably) and the bottom one "Common"; middle taps (impedance labels) not legible. No impedance numbers readable.
- Global negative feedback: **27K** (printed "27K") from the top of the OT secondary (the wire running across the top of the drawing) to the NFB line. The NFB line goes: 27K -> node -> **5K** presence pot to ground with **0.1** cap from the pot wiper to ground (drawn from wiper to the pot bottom/ground); also the **10K** to the PI junction (section 5) and the PI lower-grid 0.1 cap.

## 7. Power supply
- Mains 110-250 V 50/60 Hz, 2A slow blow, switch, mains tap selector (4 tap lugs drawn on the primary).
- Rectifier V7 (label blobbed, "GZ34" most likely (?)); anode windings labelled "360 AC" per side in the chart (V7 pin 4 and pin 6 "360 AC"). Chart V7 pin 8 (cathode) **450 V**. Heater winding separate, drawn with leads to V7 pins.
- HT after the rectifier: circled **450V** node, with a **32 uF** (value blobbed "32"/"2L"; electrolytic, + up) reservoir (?), **.05** cap from the rectifier-cathode side to ground (across the standby switch contacts), **STANDBY** switch, then the **20H choke** ("20H" printed) whose output node is circled **440V**; the output transformer centre tap and the **1K 2W** screen feed hang off the 440V node. Another electrolytic (value read "32"/"16" blobbed) sits from the 440V node to ground (?).
- **8.2K 1W** (printed "8.2K 1W") from the 440V node to the **380V** node, with a **16 uF** electrolytic (printed "16") from the 380V node to ground: 380V feeds the PI plate loads (82K, 100K) and the V2 (tremolo) supply.
- **10K 1W** (printed "10K 1W") from the 380V node to the **310V** node, with **16 uF** (printed "16", + up) from 310V to ground: 310V feeds V1 plate loads (180K, 100K), V3A plate load (100K), V3B plate.
- Hand-written HT label at the choke: "20H".
- Rectifier-less alternative: see the printed note (IN4007 diodes with EL34).
- Bias winding: tap at the HT secondary through diode and 150K (see 6).

## 8. Valve voltage chart (printed completely)
Heading: "VALVE VOLTAGE CHART  PINS"

| Valve | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|
| V1 | 220 | 0 | 1.6 | (heater, "3V AC HEATER" across 4/5 area in the V1/V2 rows, blobbed) | - | 220 | 0 | 1.6 | "3V AC HEATER" (?6.3) |
| V2 | 140 | 0 | 1.2 | "3VOF AC HEATER" (blobbed, ? "6.3V AC HEATER") | - | (crossed out) | (crossed out) | (crossed out) | "3V AC HEATER" |
| V3 | 190 | 0 | 1.1 | (heater) | - | 310 | 190 | 190 | "3V AC" |
| V4 | 250 | + | 40 | (heater) | - | 250 | + | 40 | "HEATER" |
| V5 | (crossed) | "3V AC HEATER" | 430 | 440 | "1 AMP 5 +"? (?) | (crossed) | "3V AC HEATER" | (crossed) | (crossed) |
| V6 | (crossed) | (crossed or heater) | 430 | 440 | "1 AMP 5 +"? (?) | (crossed) | "3V AC HEATER" | (crossed) | (crossed) |
| V7 | (crossed) | "HEATER" | (crossed) | "360 AC" | - | "360 AC" | - | 450 | (crossed) |

Notes: in V5/V6, pin 3 = plate 430, pin 4 = screen 440 (octal numbering: 3 anode, 4 screen, 5 grid, 8 cathode); the pin-5 text (grid) is not legible. Voltages are all to chassis, no signal.

## 9. Not legible / uncertain
- Output grid resistors (22K/33K?), PI plate-to-plate cap value, bias cap value, reservoir/choke-output electrolytic values (32 uF? / 16 uF), 56pF vs 5.6pF at the mixer, 8.3K in tremolo, OT secondary tap labels.
- Which volume pot (left/right) carries which channel name; the schematic does not label Normal/Bright.
- Pot tapers not printed.
- Exact rectifier tube type label.
- Whether the tremolo "OOK" resistor is 100K and what the 2M pot is (speed).
