# Marshall JTM45 Super PA, Type 1963 - schematic read

- Source file: `1963.gif` (1 page, 1061x794, low-res scan of a hand-lettered drawing; 2.2-4.5x zoomed crops used)
- Title printed (top left): "TYPE 1963."  No other title text, no date, no revision.
- Notes printed bottom left: "ALL RESISTORS ARE HALF WATT UNLESS OTHERWISE STATED / ALL VOLTAGES MEASURED TO CHASSIS UNDER NO SIGNAL CONDITIONS WITH AN AVO MODEL 8 MK II METER SENSITIVITY 20,000 ohm/V".
- Handwritten note next to the chart (not legible): "MAINS FUSE ?  AMP" and a second line ending "... 1 AMP"/"1.0 AMP" (?).
- Mains block: "ac. Mains 110V-250V 50/60 cps", "2A SLOW BLOW" on the mains tap, "500mA SLOW BLOW" HT fuse.
- Units: capacitors without unit: ".02", "0.02", ".1", "0.1", ".01" = uF; "270pf" = 270 pF; electrolytics "250 uF", "16", "32" (uF, + marked). K = kohm, M = Mohm.
- Circled voltages: **280**, **350**, **440V** (printed "4 4 0V"), **450V**, **10 5V** (bias point). The sheet is the 4-input (two ECC83 V1/V2) "Super PA" preamp feeding the same tone stack / PI / output as the 1961-62 tremolo chassis (no tremolo on this sheet).

## 1. Tube complement
| Pos | Tube | Function |
|---|---|---|
| V1 | ECC83 (printed with half labels V1A, V1B) | Two input channels (one per half) |
| V2 | ECC83 (V2A, V2B) | Two more input channels |
| V3 | ECC83 (V3A, V3B) | V3A gain/mixer stage, V3B cathode follower |
| V4 | ECC83 (V4A, V4B) | Long-tailed phase inverter |
| V5, V6 | output pentodes; type label not printed on the sheet | Push-pull output (voltage chart: pin 3 plate 430, pin 4 screen 440, pin 5 approx. bias; octal) |
| V7 | rectifier (not named; drawn as a full-wave tube) | Rectifier; chart "350 AC" per anode |

Heaters: "3 VOLT AC HEATERS" printed on the chart for the ECC83s (the leading "6." is probably cut; read as printed) (?).

## 2. Preamp (V1, V2: four channels)
### Inputs
- Four input pairs of jacks (8 jacks). Each pair: jack 1 and jack 2 each through **68K** to the same grid node; **1M** from the jack contact node to ground. (Same wiring as 1961/1962.)
- V1 top pair -> V1A grid, V1 lower pair -> V1B grid; V2 upper pair -> V2A grid; V2 lower pair -> V2B grid.

### Cathodes
- V1 (both halves): **820** (printed "820" followed by a stray character that looks like K, but the 1.6 V chart reading means ohms) in parallel with **250 uF** (printed "250 uF", + at the top); one common cathode network per tube.
- V2 (both halves): **820** // **250 uF**, the same.
- Chart voltages: V1 pin 3 and pin 8 **1.6 V**; V2 pin 3 and pin 8 **1.6 V**.

### Plates and coupling
- V1A, V1B, V2A, V2B plate loads: **100K** each (printed "100 K"). Plate voltages: V1 pins 1/6 **210 V**; V2 pins 1/6 **210 V** (chart prints 210 for both halves of both tubes).
- Plate -> **0.02** (printed "0.02") coupling cap to the top of a **1M** volume pot, one pot per channel (four pots). No bright caps are drawn anywhere on this sheet.
- Each pot wiper -> **470K** (printed "470 K") -> a common mix line that feeds the V3A grid. (Four 470K resistors, one per channel.)

### V3A
- Plate load **100K** ("100K"), plate (pin 1) **165 V**. Cathode **820** (to ground, not bypassed), pin 3 **1 V**. Grid pin 2 **0**.
- V3A plate direct-coupled to V3B grid (chart pin 7 **165 V**).
### V3B cathode follower
- Plate pin 6 **300 V**; cathode resistor **100K**, cathode pin 8 **170 V**. Plate node is the circled **280** supply.

## 3. Tone stack (identical to the 1961/1962 sheet)
- From V3B cathode: **270pf** from the stack input to the top of the TREBLE pot; **56K** from the stack input down to a node; **.02** from that node to the junction treble-pot bottom / bass-pot top; **250K** treble pot (wiper = output); **1M** bass pot (rheostat, wiper tied to the upper end); **25K** middle pot to ground, its wiper through **.02** back to the 56K node.
- Output: treble wiper -> **.02** -> V4A grid.

## 4. Phase inverter V4
- Upper grid via **.02**; **1M** from upper grid and **1M** from lower grid to a junction; junction -> **10K** to the NFB line, and **470** (printed "470") to the PI cathodes. Lower grid via **0.1** to the NFB line.
- Plate loads: **82K** (V4A, printed "82K"/"83K" - read 82K by comparison with the 1961 sheet) and **100K** (V4B). Plate voltages pin 1 and pin 6: **240 V** each.
- Cathodes (pin 3, pin 8): chart prints "1.4" (pin 3) and "11"/"44" (pin 8), blobbed - read as about 44 V (?). Grids printed "+".
- A small capacitor is drawn between the plates (value blobbed, "47pF"? ) (?).
- Output coupling: **0.1** from each plate to each output grid.

## 5. Output stage
- V5, V6 push-pull pentodes (tube name not printed). Grid leaks: two resistors in series, one per grid to a centre bias node; labels read "22K"/"220K" (the lower one looks like "2 2 0 K", the upper one blobbed "33K"?) (?).
- Screen resistors: **470 ohm 1W** each ("470 1W"), to a screen node, which is fed via **1K 2W** ("1K 2W") from the HT after the choke. Chart screens pin 4: **440 V** both tubes; plates pin 3: **430 V** (V5 "430", V6 "430").
- Bias (chart): pin 5 of V5, V6: "APPROX ?35" handwritten and overwritten (sign and digit not clear) (?).
- Bias network (as in the 1961): rectifier-anode tap -> diode -> **150K** ("1?0K") -> electrolytic (+/- drawn) ; divider **56K** ("5 6K") and **15K** ("15K"); small cap on the cathode line printed "8"(?) uF. The circled **10 5V** points at the bias line top node (sign not drawn) (?).
- Output transformer (taps printed): "WHITE 100V" (top tap, a 100 V line tap), "BLUE 16 ohm", "GREEN 8 ohm" (the digit and "ohm" mark partially readable: "8 O"), "YELLOW" and "COMMON". The feedback wire from the top of the sheet comes down on the right to the BLUE 16 ohm terminal.
- Global feedback: **27K** from the 16 ohm tap to the NFB line, **5K** presence pot to ground with **.01** cap (printed ".01") from the pot wiper to ground. (On the 1961/1962 sheet the presence cap reads "0.1"; on this sheet it reads ".01" (?).) The PI lower-grid **0.1** cap and the **10K** also tie to the NFB line.

## 6. Power supply
- Mains 110-250 V 50/60 cps, switch, 4 tap lugs, "2A SLOW BLOW". HT winding centre tap through the 500 mA slow-blow fuse. Rectifier V7: "350 AC" on each anode (chart pins 4 and 6), cathode (pin 8) **450 V**.
- Reservoir node, circled **450V**: **32 uF** electrolytic (printed "32", + at the bottom) and **.05** (printed "05") cap to ground; **STANDBY** switch; the choke "**20H**" and its output node circled **440V**.
- From the 440V node: **1K 2W** (screens); **8.2K 1W** (printed "8.2K 1W") to a node with **16 uF** (printed "16", + up) = the PI plate supply at circled **350**; **10K 1W** (printed "10K 1W") from the 350 node to the circled **280** node with **16 uF** (printed "16", + up). A **32 uF** (printed "32", + up) electrolytic is also on the 440V line.
- From the 280 node another series resistor (printed "1.1K"? looks like "1 . K"; blobbed) (?) goes left to the V1/V2 plate loads, with a decoupling cap symbol drawn to ground just to its left (value not legible) (?). V3A plate load and V3B plate come off the 280 node directly; V1/V2 plate loads (210 V) are behind the extra series resistor.
- Rectifier-less note from the 1961 sheet is not on this sheet.

## 7. Valve voltage chart (printed completely)
Heading "VALVE VOLTAGE CHART  PINS"

| Valve | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|
| V1 | 210 | 0 | 1.6 | "3 VOLT AC HEATERS" across 4-5 (rotated text) | (same text) | 210 | 0 | 1.6 | "3 VOLT AC HEATERS" |
| V2 | 210 | 0 | 1.6 | (heater text) | (heater text) | 210 | 0 | 1.6 | (heater text) |
| V3 | 165 | 0 | 1 | | | 300 | 165 | 170 | |
| V4 | 240 | + | 1.4 (? 4.4) | | | 240 | 1 (? "+") | 11 (? 44) | |
| V5 | (crossed) | "3 VOLT AC HEATER" | 430 | 440 | "APPROX ?35" | (blank) | "3V AC HEATER" | (crossed) | (crossed) |
| V6 | (crossed) | "3 VOLT AC HEATER" | 430 ("130" blobbed) | 440 | "APPROX ?35" | (blank) | "3V AC HEATER" | (crossed) | (crossed) |
| V7 | (crossed) | "HEATER" | (crossed) | "350 AC" | (crossed) | "350 AC" | (blank) | 450 | (crossed) |

## 8. Not legible / uncertain
- Output tube type; output grid-leak values (220K?); PI plate-to-plate cap; PI cathode voltage (1.4/4.4/44?); bias pin-5 value and sign; presence cap (.01 vs 0.1); extra series resistor on the V1/V2 supply (1.1K?); handwritten fuse note; OT secondary 8 ohm digit; no bright caps drawn (that is as drawn, not "not legible").
