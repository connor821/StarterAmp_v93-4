# Mesa/Boogie Mark I (hand-drawn schematic, "four stage pre-amp")

Source image: `mk1-1.png` (2655x2065, a low-resolution hand-drawn sheet; the top edge is cut off, so the input-jack switching and the top of the V1 area are partly missing). Values marked (?) are not reliable. Nothing here was invented. Resistors are in ohms, K = kilo-ohm, caps as printed (the sheet writes ".047" = 0.047 uF, "110pf" etc.).

## 1. Tube complement

| Position | Tube | Role |
|---|---|---|
| V1 (circled 1) | 12AX7 | Input stage, Input 1 and Input 2 jacks, Volume 1 |
| V2 (circled 2) | 12AX7 | Second stage, drives the tone stack |
| V3 (circled 3) | 12AX7 | Third stage after Volume 2 (this is also where the "optional graphic EQ" jumper sits) |
| V4 (circled 4, "A") | 12AX7 | Fourth stage, then Master 1M |
| Reverb stages (R1, R2, "RD") | 12AX7 / 12AT7-type (?) | Optional reverb driver and recovery (Hammond delay line) |
| Driver / PI (circled 5,6) | 12AX7 (printed "12AX7") | Driver and phase inverter |
| Output | four 6L6 (octal pin numbers 5, 4, 3, 8 shown) | Pushed power amp, 4 tubes |
| Rectifier | solid-state diodes | See supply |

Heaters: 2.5 V and 12.6 V style markings are printed beside V1 and V2 ("2.5v" on each triode half; this is a pin-by-pin heater note, not a plate value).

## 2. Preamp stage by stage

### V1 (input)
- Two inputs. Input 1: 1 meg grid leak (printed "1 meg") to ground. Input 2: 220K shown to ground, and a Volume 1 pot of 1 meg is drawn between the input network and V1 grid (the input switching at the top edge is not visible, so which input uses the pot divider is (?)).
- Plate load 100K (printed "100K" beside pin 1), plate voltage printed 115 V.
- Cathode: parallel "22/15" (22 uF, 15 V) with a "1.5K 1/2 watt resistors" note and a "1N5303 w/ 12AX7" note. The 1N5303 is a current-regulator diode. The note reads as: a 1N5303 current-limiting diode is drawn in the cathode of V1 and of V2 together with the 22 uF bypass, and the 1.5K 1/2 W resistors are the alternative (retrofit) cathode resistors. Exact arrangement (diode in series with the cathode or diode as the alternative) is (?).
- Coupling cap out of V1: .01 uF (printed ".01") toward the Volume pot / V2 grid.

### V2
- Plate load 100K (the number is partly struck; "100K" with a hand-drawn "X" through the first reading, so (?) whether the original value was different), plate voltage 115 V.
- Cathode: 22/15 with the same 1N5303 / 1.5K note as V1.
- Plate drives the tone stack through the treble cap.

### Tone stack (TMB variant)
Printed values:
- Treble pot 250K ("TREB 250K")
- Bass pot 250K ("BASS 250K")
- Mid pot 10K ("MID 10K")
- Treble cap: 250 pF (printed ".250" marking near the 115 V plate node; read as 250 pF, (?) about whether it could be 250 pF versus 0.250 pF)
- Bass/mid caps: .1 uF (treble bottom to bass top) and .047 uF (to the mid pot)
- Slope resistor: 100K (vertical resistor beside the stack)
- Mid pot bottom goes to a "Gain / Boost (V1)" footswitch jack ("FT SW"): a Lead Boost footswitch that selects the gain/boost state.
- A 100K series resistor follows the stack into Volume 2 (1 meg) with a 110 pF bright cap ("110pf  BRIGHT") across the top of Volume 2.

Computed with my own nodal solver of the plugin topology (source 38K = 100K plate load || ~62K estimated V2 rp-ish, load = 100K + 1M volume; all pots at linear position because the taper is not printed). These are estimates, not measurements:
- Noon (all knobs 50 %): mean insertion loss over 100 Hz to 5 kHz about 18 dB, 1 kHz about -18.6 dB, notch about -26 dB near 400 Hz.
- All at 10: minimum -23 dB near 280 Hz, maximum about -4 dB at the top.
- Treble 0 / bass 0 / mid 5: roughly flat -25 to -27 dB (stack loss with nothing to pass).
- This matches the topology the plugin uses for the Mark IV bright stack (type 11) except that the treble cap here is 250 pF versus 1000 pF in `ToneStackTmb.h` type 11; see `Amps_batch2_vs_code.md`.

### V3
- Plate 100K (printed ".047" coupling cap and 100K near the "optional graphic EQ" jumper nodes), plate voltage printed 116 V (?).
- Cathode: bypassed "21/16"-style parallel RC (printed as "21/26"/"22/16", (?) exactly) with a 1.5K (printed "5K"/"1.5K", (?)) resistor.
- Output goes to the series-through jumper "N ... G" for the optional graphic EQ. The graphic EQ itself is on the same sheet (section 5).
- The V3 plate also feeds a 10 pF with 2.2 meg parallel network toward the master/V4 area (this is a small high-frequency roll-off or bright arrangement; its exact node is (?)).

### V4 and Master
- Grid fed through a 220K resistor and a .047 uF coupling cap from the preamp signal path; the Master 1 meg pot sits at the preamp output ("PRE AMP SIGNAL OUT to Y line jack and driver").
- Plate load 100K (printed), plate voltage printed 119 V (?). Cathode "620"/"820" ohm (?) bypassed with 21/16-style cap.
- A 910K-class resistor (printed "610K"/"910K", (?)) from the plate/output node to ground.
- Reverb: optional. A 220K or 330K ("220K or 330K") divides into a 100K Reverb pot, .003 uF coupling, then reverb driver tube, Hammond delay line, recovery tube with 680 ohm cathode ("680" printed) and a .001 / 3KV (?) cap in the heater area, with 10K dropper. Mixing resistors printed 220K or 330K.

## 3. Channel switching and gain structure
- Single channel. Gain structure: V1 (Volume 1) -> V2 -> tone stack -> V3 (Volume 2) -> V4 -> Master. Volume 1 and Volume 2 together set the lead/clean split, so the Mark I has the classic two-volume cascade that the plugin's "Marque Dream, Vintage" voicing approximates.
- The footswitch jack ties the mid pot bottom through a switch: "Gain / Boost" (a Lead Boost via the mid pot's lower end).
- No clipper diodes in the preamp; all gain is tube saturation.

## 4. Driver, phase inverter and power amp
- Driver/PI: one 12AX7 ("5,6" circled; a long-tail style stage). Printed values: 56K (from the NFB node), 15K, 82K, 1 meg, 10K, 220K x2 as plate loads, 330 ohm at the lower cathode node (printed "230"/"330", (?)), a .001 signal-in cap, and a 1 meg + 10 + 150K divider at the input.
- Plate loads 220K (two, printed "220K" at both plates), with 82K and 1 meg shown on the cross network. These are the driver plate resistors for a four-tube pair set.
- Coupling: .1 uF caps to each pair of output tubes.
- Output tubes: 6L6 x4 (two pairs). Each pair has:
  - grid stopper 2200 ohm (printed "2200", i.e. 2.2K) at pin 5 (control grid),
  - screen resistor 470 ohm, 1 W (printed "470" with "1w") at pin 4 (screen).
  This is the reverse of what the older code doc reads (it states 470 ohm grid stoppers and 2.2k resistors); the octal pin numbers on the drawing put the 2200 on pin 5 (grid) and the 470 1 W on pin 4 (screen). See comparison file.
- Bias: a 1500 / 410 / EQ power -37 V tap on the "F" white line and the "C" blue (bias) lead; bias leg 1500 ohm, 410 ohm (?). The bias supply is a half-wave rectified winding (a diode and 410 ohm printed). Fixed bias voltage magnitude is not printed (?).
- "60 / 100" switch: a 100 ohm (?) resistor to ground through a switch near the output pair (a standby/bias test or "pull" switch), (?).
- Output transformer taps: "SPKR OUT" with an 8 ohm and "FB" feedback taps; taps printed 4 ohm / 8 ohm / "FB" and "RED" "A" labels; primary center tap supply labelled "449" (B+ near 449 V?), plate lines "448/449" (?). The B+ numbers I could read: 449 V (primary center) and 260 V on the "D" brown line.
- NFB: 56K from the FB tap into the PI input node, with a "PRESENCE 10K" pot ("PRESENCE" in front of a 10K, with a small cap on the pot) in the NFB branch. Presence pot printed 10K (?). Cap values around the presence control are (?).

## 5. Graphic equalizer (optional, separate board)
Five-band active EQ with three transistors and one pass transistor:
- Bands labelled by center: 80, 240, 750, 2200 Hz and a "Cut 5000" label (a fifth band / top cut), plus a "Boost" slider. Inductor/cap values in each series-LC branch: 1 H with 470?/.1 uF-ish elements; the legible ones: ".1 H" and "470", ".33 H" "470" ".1", ".22 H" ".1", ".068 H" "1K" ".011". Exact values are low resolution (?).
- Transistors: Q1 to Q4 (types not printed). Resistors: 3.3K, 82K, 150K, 3.3K, 33K, 22K, 3.3K, 3.3K, 10K, 10K (several repeated); caps .1 uF, 10 pF, 220 uF/63 V. Supply -37 V from the "F" EQ power line.
- Input resistor 10K from "G" (SIG IN), output via a cap to "SIG OUT", 10K load.
- Footswitch ("FT SW") bypass.

## 6. Supply
- Rectifier: solid-state diodes (4 drawn), standby switch ("STBY"), a 1 kohm-class wirewound string (printed "150K 1W (1)" for two bleeder resistors across the filter caps, 60/350 (?) cap labels).
- Filter: 60 uF/500 V on the "A" red line; two further series caps (printed "6.0/330 (1)" i.e. 60 uF/330 V, (?)) and a "33 uF" with "5ml" (?) on the "C" and "D" lines.
- Dropper resistors: "D" BRN through 10K, "C" YEL through 6.8K (printed "66K"/"6.8K", (?)), 100 ohm resistors (two) in the heater/bleed path.
- Printed node voltages: "A" red (main B+) about 449 V (?), "B" orange / "C" yellow / "D" brown stepped down, and 260 V at "D".
- Fuse: "2.5 slo blo" (printed text partly illegible: "2.5 amp / 3.0 hundred? slo blo", (?)).
- Bias supply as above.

## 7. Illegible or uncertain
- Top edge of the page is cut off (input jack switching, "56K w/teflon" label and a note about "Input 2").
- Exact cathode R in V3 and V4; whether V2 plate load is 100K or an alternative.
- Presence pot value and NFB node details, the bias voltage, heater voltages, B+ lines (only 449 V and 260 V were reasonably read).
- Graphic EQ inductor and cap values.
