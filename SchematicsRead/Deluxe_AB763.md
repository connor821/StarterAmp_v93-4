# Fender Deluxe-Amp AB763 (blackface, NO reverb) - schematic read

Source: `b7b32574-fender_deluxe_ab763.pdf`, 2 pages (p1 schematic, p2 chassis layout), 1-bit scans. Title on sheet: "FENDER MODEL DELUXE-AMP AB763 SCHEMATIC NOTICE", "C-FD", "Fender Electric Instrument Co., Fullerton, California U.S.A.". Header notes: voltages read to ground with electronic voltmeter, +/-20 %; resistors 1/2 W 10 % unless specified; caps at least 400 V unless specified.
Convention: Fender "1500" = 1.5k, "6800" = 6.8k, "2700" = 2.7k, "820" = 820 ohm; ".00025" = 250 pF; "470/1W" = 470 ohm 1 W. Pot tags "2-35" are pot-code tags. "A" = audio taper, "L" = linear, "RA" = reverse audio.

This is a different circuit from the Deluxe Reverb: there is **no reverb tank, no V3/V4 mixer stages**, and the B+ is ~410 V (6V6GT) with preamp plates at +170 V from a +230 V node.

## 1. Tube complement and heaters
| Position | Tube | Function |
|---|---|---|
| Vibrato channel | 7025 (one tube, V1/V2 style) | V-A and V-B stages |
| Normal channel | 7025 | N-A and N-B stages |
| Oscillator | 12AX7 (printed "12AX7") | tremolo oscillator + lamp driver |
| Phase inverter | 12AT7 | long-tailed pair |
| Output | 2 x 6V6GT | push-pull, fixed bias |
| Rectifier | GZ34 | full wave |
- Heaters: 6.3 V winding, centre tap grounded (arrow), "TO ALL 6.3 VOLT HEATERS AND PILOT LIGHT". Layout page tube row (left to right): GZ34, 6V6GT, 6V6GT, 12AT7, 12AX7, 7025, 7025.
- Mains: AC receptacle, ground switch with .047 uF 600 V, AC switch, **1 amp slo-blo fuse** (as printed).

## 2. Stage by stage
### Normal channel (7025, lower of the two 7025s on the sheet)
- **Input**: jacks 1 and 2; each via 68k into the common grid node (jack 1 -> 68k, jack 2 -> 68k); 1M from the jack-1 node to ground.
- **Stage A**: plate +170 V; plate load 100k to the **+230 V** line (node "B"); cathode 1500 ohm || 25 uF, +1.6 V.
- **Tone stack**: see section 3.
- **Volume**: 1M-A (2-35). No bright cap on Normal.
- **Stage B**: plate +170 V; plate load 100k to +230 V; cathode 820 ohm || 25 uF, +1.6 V (node "C", shared with the Vibrato channel B-stage cathode).
- **Output coupling**: **.047 uF** -> **220k** series -> PI grid node (the node feeding the PI .001 uF).

### Vibrato channel (7025, upper)
- **Input** identical (68k/68k, 1M).
- **Stage A**: plate +170 V; 100k to +230 V; cathode 1500 ohm || 25 uF, +1.6 V.
- **Volume** 1M-A (2-35); **bright cap 47 pF** across the upper half of the volume pot (printed "47 PF").
- **Stage B**: plate +170 V; 100k to +230 V; cathode +1.6 V to node "C" (shared 820 ohm || 25 uF, same node as Normal stage B).
- **Output**: **.1 uF** -> **220k** -> PI grid node.
- The two 220k mixers sum at the PI grid node, so there is no V4 mixer tube.

## 3. Tone stack (both channels)
- Plate node -> **.00025 uF (250 pF)** -> top of TREBLE pot 250K-A.
- Plate node -> 100k -> stack node; .1 uF from stack node to bass top (= treble bottom); .047 uF from stack node to bass bottom; bass bottom -> **6800 ohm to ground**.
- TREBLE 250K-A (2-35), BASS 250K-A (2-35) wired as variable resistor, VOLUME 1M-A.
- Same standard TMB-without-mid topology as the Deluxe Reverb.

## 4. Reverb
None.

## 5. Tremolo ("Vibrato"), V5 12AX7
- Oscillator: V5A plate **+250 V**, plate load **220k**; cathode **2700 ohm || 25 uF**, +2.2 V.
- Modulator V5B: plate **+340 V**, cathode **100k || 25 uF**, cathode printed +17.0 V. Plate path: **10M** to the supply and a neon lamp in series with **100k** (lamp+photocell opto drawn as one assembly; the photo-resistor leg goes to the **50K-RA INTENSITY** pot to ground).
- Phase-shift network values: 1M, 1M, 2.2M, .01, .01, .02, SPEED = **3M-RA** pot in series with **100k** to ground (the 3M/100k pair is the variable leg). Exact node-by-node wiring (?) - values certain, topology only as drawn.
- Pedal jack (switch, vibrato on/off) at the oscillator input node (junction of the 1M and 2.2M).
- Photocell connection to the audio path: the opto element's other terminal is drawn to an "up arrow" connector that ties to the Vibrato-channel output node (after its .1 uF) ahead of the 220k; i.e. the photocell + intensity pot shunt that channel's output (?). Normal channel is not modulated.

## 6. Phase inverter, 12AT7 long-tailed pair
- Input: .001 uF from the two-220k mix node to V6A grid; 1M from V6A grid to the +88.5 V node.
- V6B grid: .1 uF (200 V, printed ".1-200") from the NFB node; 1M to the +88.5 V node.
- Cathodes common, +90 V, via **470 ohm** to the +88.5 V node; +88.5 V node via **22k** to the NFB node; NFB node -> **47 ohm** to ground.
- Plate loads: **82k 5 %** (V6A) and **100k 5 %** (V6B) from the +340 V line (junction of the two); V6A plate +180 V, V6B plate +180 V.
- **NFB**: **820 ohm** from the output-transformer secondary (speaker line) to the NFB node (47 ohm to ground, 22k to the PI cathode network).
- Output coupling .1 uF to each 6V6 grid.

## 7. Output stage
- 2 x 6V6GT push-pull, fixed bias, cathodes to ground.
- Printed: plates **+410 V**, screens **+405 V** (via **470 ohm 1 W** each from the +410 V screen line), OT centre tap/screen line **+410 V**; "+420 V" printed at the rectifier/choke input node.
- Grid stoppers **1500** each; grid leaks **220k 5 %** each to the bias line.
- **Bias**: **-55 V** printed on the bias line (schematic). Chain: 325 VAC lead -> **470 ohm 1 W** -> diode (marked "-" pot side, "+" 470 side) -> **25/50 uF** (25 uF 50 V) -> **10K-L** adjust pot with 10k to ground on one end; wiper = bias line. The layout page prints "-35 V" at the bias pot - **schematic -55 V vs layout -35 V disagree**; schematic value used.
- OT: **TR3** (export P/N 125A1A). Primary Z not printed; speaker symbol only (impedance not printed) (?). EXT SPKR jack.
- No power-tube cathode resistor; no 1200 pF grid caps.

## 8. Supply
- GZ34, two "325 VAC" legs.
- Filter: rectifier -> STANDBY switch -> **16 uF 450 V** -> node (+420 V) -> choke **TR2** (P/N 125C3A) -> node "A" **16 uF 450 V** -> **10k** -> node (+340 V; supplies PI plates and V5B) **16 uF 450 V** -> **27k** -> node "B" (+230 V; preamp plates and V5A) **16 uF 450 V**.
- Printed: +420 V (rectifier/choke input), +410 V (plates, screen/CT line), +340 V, +230 V; preamp plates +170 V; V5A +250 V, V5B +340 V; PI cathodes +90 V.
- Only one big reservoir cap before the choke (the Deluxe Reverb has two paralleled).

## 9. Not legible / uncertain
- Wattages of preamp plate resistors (shown "100K" only, 1/2 W by the sheet default).
- Photocell/lamp connectivity and the 10M/100k placement around the lamp; phase-shift network node wiring.
- Which node the OT centre tap hangs on (shown tied to the +410 V line; +420 V label is at the choke input).
- OT primary impedance and speaker impedance not printed.
- Bias value discrepancy: -55 V (schematic) vs -35 V (layout, p2).
