# Fender Vibrolux Reverb AB568 - schematic read

Source: c7cc317c-Fender_Vibrolux_Reverb_Amp_AB568.pdf (1 page, raster 898x661 px only; "Fender Musical Instruments / CBS ... Santa Ana, California"; patents 2817708, 2973681, 192859 and pending). Resolution is poor; small print (all voltage labels, some pot codes, capacitor values in pF) is at the limit. Marked (?) = less than sure.
Notice on drawing: 1 voltages read to ground with electronic voltmeter, values +/-20 %; 2 all resistors 1/2 W 10 % unless specified; 3 all capacitors at least 400 V unless specified; 4 "vibrato grounded" (voltages taken with the vibrato pedal jack grounded).
Channels: NORMAL and VIBRATO (the Vibrato channel carries the reverb/vibrato circuits). Same board layout as the AB763 Vibroverb, which is why so much matches.

## 1. Tubes and heaters
| Position | Type |
|---|---|
| Normal channel | 7025 (stage 1 + 2) |
| Vibrato channel | 7025 (stage 1 + 2) |
| Reverb recovery + mixer | 7025 ("1/2 7025" x2, one envelope; cathodes tied at node E) |
| Phase inverter | 12AT7 |
| Reverb driver | 12AT7 (halves paralleled) |
| Vibrato | 12AX7 (oscillator + modulator) |
| Output | 2x 6L6GC |
| Rectifier | 5U4GB |
Heaters: "to all 6.3 V heaters and pilot light"; heater winding has two 100 ohm resistors to ground (artificial centre-tap, one to each side of the winding). Mains: AC switch with 2 A SLO-BLO fuse, ground switch with .047 uF/600 V cap. Export transformer table: TR1 125P31A / 125P26A, TR2 125C3A, TR3 125A6A (read), TR4 125A20B.

## 2. Preamp stages
### Normal channel
- Inputs jacks 1 and 2: 68K and 68K (read "68K, 68K"), jack-1 contact resistor 1M to ground (reads "1M").
- Stage N1 (7025a): plate load 100K to the +310 V rail; plate printed +200 V (?); cathode 1500 ohm + 25 uF bypass.
- Tone stack (section 3).
- Stage N2 (7025b): cathode shared at tie "A" with the vibrato V2 cathode network (820 ohm + 25 uF); plate load 100K to +310 V; plate printed +200 V (?). Output .047 uF then 220K to the mix node (the PI input node).
### Vibrato channel
- Same inputs (68K, 68K, 1M).
- V1: plate 100K to +310 V, plate +200 V (?); cathode 1500 + 25 uF, printed +1.6 V (?).
- V2: plate 100K load, plate printed about +200 V (?); cathode 820 ohm + 25 uF with +1.5 V printed (tie A). Coupling .02 uF.
- V2 plate output also feeds 500 pF to the reverb driver grid line and the 10 pF || 3.3M network (as AB763) into the mixer grid.

## 3. Tone stack (both channels)
- 250 pF plate -> treble pot top; 100K plate -> stack node; .1 uF stack node -> bass top; .047 uF stack node -> bass bottom; 6800 ohm bass bottom -> ground.
- Pots: TREBLE 250K-A (code "2-35"), BASS 250K-A, VOLUME 1M-A (code "2-35") (?).
- Bright: 120 pF across the volume pot via the BRIGHT switch, both channels.

## 4. Reverb and vibrato
- Reverb driver 12AT7, halves paralleled: grid via 500 pF from the vibrato V2 plate; 1M grid leak to ground; cathode 2200 ohm + 25 uF; printed cathode +8.5 V (?); plate +425 V printed at the TR4 primary; TR4 primary other end to node B (+460 V). TR4 secondary -> reverb unit input. Impedances/ratio not printed.
- Recovery (7025 half): reverb output -> grid; 220K from the grid to ground and the REVERB PEDAL switch; cathode 820 ohm + 25 uF, +1.7 V (node E); plate +200 V printed (?), plate load 100K to +310 V node D.
- Plate -> .005 uF -> REVERB 100K-L pot to ground; wiper -> 470K -> node with 220K to ground -> mixer grid line (this is the same ladder as AB763, which uses .003 uF here instead of .005 uF).
- Mixer half: same as AB763 (plate 100K/220K/.047/.1 network).
- Vibrato (12AX7): oscillator network 1M, 1M, .01, .01 (?), .02 uF, 2.2M; cathode 2700 ohm + 25 uF (+2 V printed); plate +260 V; plate load 220K; SPEED 3M-RA (?) with 100K; modulator: plate +360 V printed, 10M to B+, cathode 100K + 25 uF (+10 V (?)), neon/photocell ellipse, 100K to B+; INTENSITY 50K-RA (read "50K-RA 2-35"): its top end taps the 10K/10K/15K bias divider (the -41 V bias line) and the photocell connects to its wiper, i.e. bias modulation of the 6L6 grids. The 27K on this drawing is the rectifier-tap series resistor of the bias supply (not a shunt to ground). Vibrato pedal jack shown.

## 5. Phase inverter (12AT7, long-tail)
- Input: mix node -> .001 uF -> top grid; 1M grid leak each grid to node J; bottom grid .1 uF (".1-200") to the left bus; left bus: 100 ohm to ground (read "100"), 820 ohm NFB to the speaker; 22K from left bus to J; 470 ohm J to cathodes; cathodes printed +40 V (reads "40OV"; could be +40 V (?)).
- Plate loads: 47K 5 % each (top and bottom) from the node fed by the +375 V line (the +375 V tap between 4.7K and 10K). Plates printed about +270 V (both).
- Plate outputs -> .1 uF each -> grid stoppers (1500 ohm) -> 6L6 grids. 100K grid leaks to the bias line.

## 6. Output stage
- 2x 6L6GC, push-pull, with BOTH a negative grid-bias line and cathode resistors (this is the main difference from the Vibroverb):
  - Cathodes: two cathode resistors, 7 W each, printed "120" (top) and "150" (bottom) (digits ambiguous; likely both the same value (?)) to a centre node that goes to ground; a 25 uF/50 V (?) non-polarised-looking capacitor sits between the two cathodes (read "25 ... 50V"). Cathode voltages printed about +10 V and +10 V (reads "+10V", "+10V").
  - Grid network: coupling .1 uF, 1500 ohm stoppers, 100K grid leaks to the bias line, plus two small caps (value illegible, hundreds of pF) from the grid lines to the cathode centre node.
  - Bias line printed -41 V (?) fed from the bias supply (below). I cannot reliably separate "-41 V" (grid line) from "-30 V" (supply, printed next to the diode).
- Screens: 470 ohm 1 W each from node B (+460 V printed).
- Plates +460 V printed on both plates; OT centre tap line printed +465 V.
- Output transformer TR3: speaker secondary with two speaker lines; two speakers drawn, each hand-annotated "8 ohm" (handwritten), plus EXT SPKR jack.
- Global NFB: 820 ohm from the speaker secondary to the PI left bus with 100 ohm to ground.
- Bias supply: 5U4GB TR1 tap -> 27K (series, from the tap) -> diode (polarity "+" right) -> 10K-L pot (bias adjust) with 25 uF/10 V? ("25 ... 10V") to ground; printed -30 V at the diode node; a 10K / 10K / 15K divider network ties the bias line (printed -41 V) around the pot.

## 7. Supply
- TR1: printed "245V AC"/"345V AC" on each side to the 5U4GB (first digit hard to read; 345 V fits the +463 V output) (?); rectifier output printed +463 V (labelled +463V on the line from the tube).
- STANDBY switch; first filter two 16 uF/450 V caps (two parallel electrolytics, about 32 uF); TR2 choke; B node +460 V with 16 uF/450 V; B -> 4.7K 1 W -> node (16 uF/450 V; +375 V printed for the PI plate tap/ screen?); node -> 10K 1 W -> D node (16 uF/450 V; +310 V printed). Preamp plates on D.
- Printed: +463 (rectifier), +460 (B), +375, +310 (D).

## 8. Not legible / uncertain
- Every small voltage label (preamp plates and cathodes); the cathode resistors "130/150"; the cathode bypass value.
- Whether the bias line is -41 V or -30 V and the exact bias-supply wiring; PI cathode voltage (+40 V?); 25 uF/50 V cathode capacitor.
- Pot codes "2-35"; speed pot value; neon/photocell connections; transformer ratios.
