# Fender Princeton Reverb AA1164 (blackface, GZ34 rectifier) - schematic read

Source: 86cffeb3-fender_princeton_reverb_gz34_aa1164.pdf, 2 pages, both scanned in portrait at 300 dpi but drawn rotated 90 degrees (rotated for reading). Page 1 = schematic "Princeton Reverb-Amp AA1164" (drawing code "K-FD", Fender Musical Instruments, Santa Ana CA, a division of Columbia Records Distribution Corp.; patents 2817708, 2973681, 192859). Page 2 = layout drawing of the same amp (chassis, wiring colours, same values: used to cross-check pot values, nothing contradicting page 1).
Notice: voltages read to ground with electronic voltmeter, +/-20 %; all resistors 1/2 W 10 % unless specified; all capacitors at least 400 V rating unless specified. Legibility is very good.
Transformers listed: TR1 125P1B (power), TR2 125A10B (output), TR3 125A20B (reverb driver transformer).

## 1. Tubes and heaters
| Position | Type | Function |
|---|---|---|
| V1 (7025) | 7025 | Stage 1 (pins 1,2,3) + stage 2 (pins 6,7,8) of the single channel |
| V2 | 12AX7 (half) | "mixer / reverb return + feedback" stage (called 1/2 12AX7 on schematic) |
| V2 other half | 12AX7 (half) | cathodyne (split-load) phase inverter |
| V3 | 12AX7 (half) | reverb recovery stage |
| V4 | 12AX7 (half, shared envelope) | vibrato oscillator |
| V5 | 12AT7 | reverb driver (halves paralleled) |
| V6, V7 | 6V6GT x2 | push-pull output |
| V8 | GZ34 | rectifier |
(Which half of a 12AX7 shares an envelope with which is not shown; the schematic only prints "1/2 12AX7" four times plus the 7025 and 12AT7; the layout page shows two 12AX7 sockets plus one 7025 and one 12AT7.) 6.3 V heaters and pilot lamp from TR1; heater wiring not drawn on the schematic page. Power: AC receptacle, ground switch with .047 uF/600 V, 1 A SLO-BLO fuse, AC switch, TR1 primary.

## 2. Preamp (single channel, two inputs)
- Inputs: jack 2 -> 68K -> grid node; jack 1 -> 68K (via the shorting contact); 1M from jack-1 contact to ground: network = 68K, 68K, 1M (cross-wired as on other Fender 2-jack amps).
- Stage 1 (7025 first triode): plate load 100K to +240 V rail D; plate +160 V; cathode 1500 ohm + 25 uF/25 V bypass; cathode +1.3 V.
- Tone stack (section 3) between stage 1 plate and the volume pot.
- Stage 2 (7025 second triode): grid from the volume wiper; plate load 100K to D (+240 V); plate +160 V; cathode 1500 ohm + 25 uF; +1.3 V (printed "+1.3 V"). 
- Stage 2 plate -> .02 uF -> node X (reverb/NFB mix node), which connects to: 3.3M || 10 pF to the grid of stage 3; the 470K from the REVERB pot wiper; and the 500 pF to the reverb driver grid.
- Stage 3 ("1/2 12AX7" at +160 V): grid at node X through 3.3M || 10 pF; plate load 100K to D; plate +160 V; cathode 1500 ohm + 25 uF/25 V (+1.2 V) then 47 ohm to ground. Global NFB: 2700 ohm from the output-transformer secondary (EXT SPKR jack line) to the junction of the 1500/25 uF and the 47 ohm (the 47 ohm is the feedback divider leg). This stage plate output goes to the PI via .02 uF.

## 3. Tone stack
- 250 pF plate -> top of treble pot; 100K plate -> stack node; .1 uF stack node -> bass pot (top); .047 uF stack node -> bass pot bottom; 6800 ohm bass bottom -> ground.
- Pots: TREBLE 250K-A (2-35), BASS 250K-A, VOLUME 1M-A (2-35). Treble wiper to top of volume pot; volume wiper to stage 2 grid. No bright cap, no mid control.

## 4. Reverb and vibrato
### Reverb driver (12AT7, halves paralleled)
- Grid fed via 500 pF from node X (stage-2 output); 1M grid leak to ground. Cathode 2200 ohm + 25 uF (cathode +8.0 V). Plate at TR3 primary (+400 V printed, supply node B). TR3 secondary -> REVERB UNIT input. No ratio/impedance printed.
### Reverb recovery (1/2 12AX7)
- REVERB UNIT output -> grid; 220K to ground; REVERB PEDAL switch grounds the grid. Cathode 1500 ohm + 25 uF/25 V (+1.2 V). Plate +160 V, load 100K to D (+240 V). Plate -> .003 uF -> REVERB pot 100K-L to ground; wiper -> 470K -> node X (the grid of stage 3 / the 3.3M).
### Vibrato (bias-modulation tremolo; 1/2 12AX7 oscillator)
- Oscillator triode: plate +260 V, plate load 220K to node B; cathode 3300 ohm + 25 uF/25 V (+2.4 V); phase-shift network: .01 uF, .01 uF, .02 uF (.02 uF also to ground) with 1M, 1M, 1M (three 1M resistors read) and a .01 uF; SPEED 3M-RA (printed "3M-RA SPEED") in series with 100K to ground; VIBRATO PEDAL jack with switch shorts the oscillator.
- Output: plate (+260 V) -> 1M -> node with .02 uF to ground -> .1 uF -> INTENSITY 250K-L pot; wiper -> bias line (-34 V). 27K from the pot's lower end to ground. So the oscillator signal rides on the 6V6 bias line.

## 5. Phase inverter
- Cathodyne (split-load) on a 1/2 12AX7: grid from the stage-3 plate via .02 uF, grid leak 1M to the cathode-resistor junction (+49 V).
- Plate load 56K to D (+240 V); plate +200 V; cathode resistor 1K (cathode +50 V at the top of 1K) in series with 56K to ground (junction +49 V).
- Outputs: plate -> .1 uF -> top 6V6 grid; cathode (top of the 1K) -> .1 uF -> bottom 6V6 grid. No grid stoppers drawn.

## 6. Output stage
- 2x 6V6GT, fixed bias (cathodes directly to ground), screens tied straight to node B (+400 V printed), plates +410 V printed on both plates; OT centre tap +420 V.
- Grid leaks: 220K 5 % to the bias node at each grid (the two 220K meet at the bias line, "-34 V").
- Output transformer TR2 (125A10B): primary with centre tap; secondary connects to the speaker and an EXT SPKR jack; one speaker; impedance not printed (?).
- NFB: 2700 ohm from the OT secondary (EXT SPKR jack node) back to the 1500/47 junction of stage 3 cathode (see section 2).
- Bias supply: TR1 winding -> 100K 5 % -> diode (negative toward the bias node) -> node with 25 uF/50 V to ground and the INTENSITY pot top; 27K from the pot bottom node to ground gives -34 V (printed). No bias trimmer on this model (fixed).

## 7. Supply
- TR1 secondary: 340 V AC - 0 - 340 V AC to GZ34 (printed 340 V AC each side). Rectified output +420 V (printed).
- Filter: 20 uF/450 V (first cap, square-marked node, +420 V; OT centre tap here); 1K 1 W -> node B 20 uF/450 V (+400 V; screens, reverb transformer, vibrato plate loads) -> 18K 1 W -> triangle node 20 uF/450 V (+320 V) -> 18K 1 W -> node D 20 uF/450 V (+240 V; preamp/PI plate loads). No choke.
- Printed: +420 (rectifier / CT), +410 (6V6 plates), +400 (screens, B, reverb driver plate), +320, +240 (D), -34 (bias).

## 8. Not legible / uncertain
- Which 12AX7 sections share an envelope; heater wiring. Output-transformer secondary impedance and speaker impedance; reverb transformer ratio/tank impedance.
- The vibrato phase-shift network is a seven-component network; the element-to-node mapping is (?), values .01/.01/.02/1M/1M/1M and the .02 to ground read clearly.
