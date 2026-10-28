# Fender Vibroverb 6G16 (brownface) - schematic read

Source: 3fb9f31d-Fender_Vibroverb_Amp_6G16.pdf (1 page, 2153x1410 px bitmap scan, drawing code D-FC; patents 2817708, 2973681, design 192859). Notice on drawing: voltages read to ground with electronic voltmeter, values shown +/-20 %; resistors 1/2 W, 10 % unless specified; capacitors at least 400 V unless specified.
Resolution is low; small print (pot tap values, some caps) is at the limit. Marked (?) = less than sure. This file was rewritten after a second, careful pass over the zoomed crops.
Ground symbols: all drawn as triangles; treated as ground.

## 1. Tubes and heaters
| Position (as drawn) | Type printed | Function |
|---|---|---|
| NORMAL channel, 2 triodes | 7025 | N1 (stage 1), N2 (stage 2) |
| BRIGHT channel, 2 triodes | 7025 | B1, B2 |
| 7025 (top centre, left of PI) | 7025 | mixer / reverb-return stage (see section 4) |
| 7025 (top right) | 7025 | long-tail phase inverter (7025, not 12AT7) |
| 12AX7 (bottom left) | 12AX7 | reverb driver, two halves paralleled, plate into TR4 |
| recovery triode (right of tank) | not labelled | reverb recovery (probably the other half of a 7025/12AX7; type not printed) |
| 12AX7 (centre right) | 12AX7 | vibrato: oscillator (left triode) + follower (right triode) |
| 2x 6L6GC | 6L6GC | push-pull output |
| rectifier | GZ34 | |
Heaters: "TO ALL 6.3 HEATERS AND PILOT LIGHT"; wiring not drawn. Mains: AC switch, 2 AMP FUSE, ground switch with .05 uF/600 V. Export transformers listed: TR1 125P31A / 125P26A, TR2 125C1A, TR3 125A6A, TR4 125A20A (TR1 power, TR2 choke, TR3 output, TR4 reverb transformer).

## 2. Preamp stages
### Normal channel
- Input: jack 2 -> 68K -> grid node; grid node -> 68K -> jack-1 contact; 1M from jack 1 to ground (read "1M"). (Same two-68K/1M cross-wired jack network as later Fender amps.)
- N1: plate printed +220 V (printed "+220V"), plate load 100K to supply node D; cathode 1500 ohm + 25 uF bypass; cathode +1.8 V printed (?).
- Tone stack (section 3) feeds N2 grid via volume pot.
- N2: plate +135 V printed, plate load 220K to the rail; cathode +1.1 V to boxed tie point "A" (cathode network shared with the Bright N2/B2 cathode, see below). Plate output goes through 220K (drawn along the top) toward the mixer stage (section 4).
### Bright channel
- Input network same as Normal (68K, 68K, 1M).
- B1: plate +210 V printed, 100K load to the rail; cathode 1500 ohm + 25 uF, +1.8 V (?).
- B2: cathode 820 ohm + 25 uF (cathode +1.1 V, tie point A). Plate load: 22K then 82K in series from the rail (plate printed "+200V" with the 82K label; exact voltage digit (?)). From the 22K/82K junction a .05 uF (?) goes through 470K to the mixer-line node (section 4). B2 plate also feeds a 500 pF to the reverb driver grid line.

## 3. Tone stack (same topology on both channels, as drawn)
- .00025 uF (250 pF) plate -> top of TREBLE pot; 100K plate -> stack node; .1 uF stack node -> BASS pot top (labelled ".1"); .05 uF stack node -> BASS pot bottom (printed ".05"); 6800 ohm bass-pot bottom -> ground.
- Pots (printed): TREBLE 250K-A with "10K TAP" printed (a 10K tap, wiper/tap usage (?)); BASS 250K-A; VOLUME 1M-A. A cap printed ".005" (read; could be ".008") from the volume pot top to ground (treble-bleed/shunt) on both channels (?).
- Bright channel only: 47 PF across the volume pot (top to wiper).
- Taper letters "A" as printed.

## 4. Reverb and vibrato
### Mixer / return stage (top-centre 7025)
- Grid line gets: reverb return via 470K from the REVERB pot wiper; B2 signal via .05 uF/470K. Cathode 1500 ohm, printed +1.5 V (no bypass drawn). Plate +210 V (read); plate node ties to the Normal-channel 220K, a 1M, and a 100K to the rail (exact interconnection (?)). The output goes through a small cap (".0005"/".005" (?)) to the PI. I could not trace this mixer cleanly in the scan.
### Reverb driver (12AX7)
- Both halves paralleled; grid via 500 pF from B2 plate, 1M grid leak; cathode 1K 1 W + 25 uF; cathode printed about +3.3 V (?); plate +430 V (read "+430V") into TR4 primary, other end to node B (+440 V).
- TR4 secondary -> REVERB UNIT INPUT (shielded). Transformer ratio / tank impedance not printed.
### Reverb recovery
- REVERB UNIT OUTPUT -> grid; 220K grid-to-ground; REVERB PEDAL switch (dashed) grounds the grid. Cathode 1500 ohm + 25 uF, +1.5 V (?); plate +210 V, 100K load to node D.
- Plate -> .003 uF -> REVERB pot 100K-L (to ground); wiper -> line up to the mixer stage via 470K.
### Vibrato (bias-modulation, 12AX7)
- Left triode = oscillator: plate +230 V, plate load 470K to B; cathode 4700 ohm + 25 uF bypass (+2.0 V). Phase-shift network of .01 uF, .01 uF, .02 uF, .08(?) uF and 1M, 1M, 1M (read). SPEED = 3M-RA in series with 100K to ground; VIBRATO PEDAL jack with switch.
- Right triode: plate +440 V directly on B, grid fed from the oscillator plate, cathode 470K to ground (second 470K also drawn from the grid line). Cathode output -> .1 uF -> INTENSITY 250K-L; a .1 uF to ground on the signal node. INTENSITY wiper goes to the -62 V bias line at the 6L6 grid-leak junction.

## 5. Phase inverter (7025, long-tail)
- Coupling from the mixer stage: small cap (".0005"? (?)); each grid has a 1M leak to node J; bottom grid also gets .1 uF/200 V ("-200") from the left bus.
- Tail: 6800 ohm from the left bus to J; 820 ohm from J to the two cathodes (cathodes +19.0 V printed); left bus: 4700 ohm to ground, and a 10K (top of drawing) global-NFB resistor to the output-transformer secondary.
- Plate loads: top 82K 5 % and bottom 100K 5 % (read) from node C; plates printed +250 V (top) and +260 V (bottom). (82K gives the lower plate voltage, consistent with the printed values.)
- Outputs: .1 uF each to the 6L6 grids.

## 6. Output stage
- 2x 6L6GC, fixed bias, cathodes straight to ground. No grid stoppers drawn.
- Grid leaks: two 220K 5 % in series, joined at the bias node (printed -62 V; read "-6.2V"/"-62V"; Fender brownface bias around -50 to -60 V, so "-62 V" is plausible but digit (?)).
- Screens: 470 ohm 1 W each from node B; B +440 V printed; plates +440 V printed on both plates and at the OT centre tap line. Choke TR2 sits before B.
- Output transformer TR3: two speaker jacks drawn (two speakers) plus EXT. SPKR jack; impedances not printed.
- Global NFB: 10K from the OT secondary back to the PI left bus (6800 / 4700 junction).
- Bias supply: tap on TR1 -> "1K-1W" variable resistor (bias adjust) -> diode (- end toward the bias node) -> node with 25 uF/50 V and 33K 1/2 W 5 % to ground; the INTENSITY pot sits across this node; node = -62 V line.

## 7. Supply
- TR1 -> GZ34, 380 V and 380 V (read "380V") AC; rectifier output +440 V (printed).
- STANDBY switch, then two 16 uF/450 V first-filter caps, choke TR2, node B 16 uF/450 V (+440 V); B -> 10K -> node C 16 uF/450 V (PI plate loads, +250/+260 V at PI plates); C -> 10K 1 W -> node D 16 uF/450 V (preamp plate loads, supply for first stages).
- Printed: +440 V (B and OT, screens), +230 V (vibrato oscillator plate), +250/+260 V (PI), preamp plates +220/+210/+135/+200 V as above. C and D rail voltages themselves are not printed.

## 8. Not legible / uncertain
- Tone-stack pot taps and the small ".005/.008" shunt cap; B2 plate resistor digits; whether the 22K/82K branch carries the .05 uF.
- The top-centre mixer stage wiring and its coupling cap value; PI coupling cap value (.0005 vs .005).
- Vibrato network cap values (.01/.02/.08?); exact bias voltage (-62 V?).
- Reverb transformer ratio, tank and speaker impedances; heater wiring.
