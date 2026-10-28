# Fender Princeton Reverb AB1270 (silverface, 5U4GB rectifier) - schematic read

Source: 2f1846a7-Fender_PRINCETON_REVERB_AB1270.pdf, 2 pages, each page stored as 13 stacked image strips (67 ppi, 1650 px wide), which I re-stitched into one image per page. Page 1 = schematic, page 2 = layout/wiring. CBS Musical Instruments, Fullerton CA; title block "Schematic & Wiring Diagram, Domestic and Export, Princeton Reverb Amp", dwg no. 045427, rel. no. 571, rev C (rev A ECO 1423 ~1/69, rev B ECO 2819 1/25/71, rev C ECO 2929 6/1/71 "added alternate transformer connections"); drawn B.R. 1969. The schematic header reads "Princeton Reverb-Amp B1270" (file name AB1270; first letter cut off on the scan (?)).
Notice: voltages read to ground with electronic voltmeter, +/-10 % (AA1164 said +/-20 %); resistors 1/2 W 10 % unless specified; capacitors at least 600 V (AA1164: 400 V). Scan is a photocopy with show-through of the back side; main text legible.
Transformers: TR1 125P1B (FMI 022772); TR2 125A10B (022913); TR3 125A20B (022921); TR1 export-only 020873; TR1 alternate dual-primary 010020.

## 1. Tubes and heaters
- V1A/V1B 7025 (stage 1 and 2 of the single channel)
- V2 12AT7 reverb driver (halves paralleled)
- V3A 1/2 12AX7A (stage 3: NFB and mixer stage), V3B 1/2 12AX7A (reverb recovery)
- V4A 1/2 12AX7A (cathodyne PI), V4B 1/2 12AX7A (vibrato oscillator)
- V5, V6 6V6GTA output pair (labels V5 and V6 appear on the drawing; the schematic text labels "V6 6V6GTA" top and "6V6GTA V5" bottom)
- V7 5U4GB rectifier
Heaters: "to all 6.3 V heaters and pilot lights", two 100 ohm resistors from the winding ends to ground (artificial centre tap); heater wiring not drawn. Power: ground switch with .047 uF/600 V, AC switch, 1 A SLO-BLO fuse, 117 V 60 Hz domestic hook-up; export hook-up (117-240 V 50/60 Hz) via tapped TR1 primary with a 1 A/250 V fuse.

## 2. Preamp
- Inputs: jack 2 -> 68K; jack 1 -> 68K (cross-wired through the jack-2 shorting contacts); 1M from jack 1 to ground.
- V1A (stage 1): plate load 100K 1 W to +240 V (D); plate +160 V; cathode 1500 ohm + 25 uF; +1.3 V.
- Tone stack: 250 pF, 100K, .1 uF, .047 uF, 6800 ohm; TREBLE 250K-A (2-35), BASS 250K-A, VOLUME 1M-A (2-35) on the schematic. (Layout page prints the same pots as 250K-J / 250K-J / 1M-J with part nos. 021857 (bass, treble), 021832 (volume), so the taper letter is A on the schematic and J on the layout; same audio taper family (?).)
- V1B (stage 2): plate load 100K 1 W to D; plate +160 V; cathode 1500 ohm + 25 uF; +1.3 V. Output .022 uF to node X.
- Node X: 3.3M || 10 pF to V3A grid; 470K from the reverb wiper; 500 pF to the reverb driver grid.
- V3A: plate load 100K 1 W to D; plate +160 V; cathode 1500 ohm + 25 uF (+1.2 V) in series with a 47 ohm resistor to ground; 2700 ohm NFB from the output-transformer secondary (EXT SPKR jack node) to the junction of the 1500/25 uF and the 47 ohm. Output .022 uF to the PI grid.

## 3. Reverb and vibrato
- Reverb driver V2 12AT7: grid 500 pF from node X, 1M grid leak; cathode 2200 ohm + 25 uF (+8.0 V); plate printed +395 V at TR3 primary (supply B); TR3 secondary to REVERB INPUT.
- Recovery V3B: REVERB OUTPUT -> grid; 220K in parallel with 2000 pF to ground at the grid (AA1164 has only 220K); REVERB PEDAL switch grounds the grid; cathode 1500 ohm + 25 uF (+1.2 V); plate +160 V, 100K 1 W to D; plate -> .003 uF -> REVERB 100K-L to ground (021873); wiper -> 470K -> node X.
- Vibrato V4B (oscillator): plate load 220K to B; plate printed +268 V and +260 V (two overlaid values (?)); cathode 3300 ohm + 25 uF (+2.4 V); network .01 uF, .01 uF, .022 uF, 1M, 1M, 1M, plus .022 uF to ground; SPEED 3M-RA + 100K (SPEED 3.0M-RA 021865 on layout); VIBRATO PEDAL jack. Output through 1M -> .1 uF -> INTENSITY 250K-L (033043) wiper onto the -34 V bias line; .022 uF to ground at the plate/1M node.

## 4. Phase inverter (V4A cathodyne)
- Grid from V3A plate via .022 uF; 1M grid leak to the cathode junction (+49 V). Plate load 56K to D; plate +200 V. Cathode: 1K (cathode +50 V) then 56K to ground (+49 V at the junction). Outputs: plate -> .1 uF -> top 6V6 grid; cathode node (top of 1K) -> .1 uF -> bottom 6V6 grid.

## 5. Output stage
- 2x 6V6GTA fixed bias, cathodes to ground; plates +410 V printed, screens +400 V printed (from B); OT centre tap +420 V.
- Grid leaks: two 220K from the grids to the -34 V bias node (no grid stoppers).
- Output transformer TR2 125A10B; secondary to the speaker and EXT SPKR jack; speaker impedance appears as "8 ohm" next to the speaker symbol (faint, (?)).
- NFB: 2700 ohm from the secondary to the V3A cathode junction.
- Bias supply: TR1 tap -> 100K 1 W -> diode 0.5 A/600 V (020881) -> 50 uF/70 V to ground; 27K from the node to ground and the 250K-L INTENSITY pot; result -34 V.

## 6. Supply
- TR1 340 V AC - 0 - 340 V AC -> 5U4GB. Rectifier output +420 V (OT CT).
- 20 uF/450 V first cap; 1K 1 W -> B (+400 V; screens, reverb transformer, vibrato plates) 20 uF/450 V; 18K 1 W -> +320 V node 20 uF/450 V; 18K 1 W -> D (+240 V) 20 uF/450 V. No choke.
- Note the AB1270 uses the same four-cap RC ladder as AA1164 (1K / 18K / 18K).

## 7. Not legible / uncertain
- Vibrato plate voltage (+268 vs +260); "B1270" first letter; speaker impedance; reverb transformer ratio and tank impedance; heater wiring.
