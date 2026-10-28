# Fender Vibroverb Amp AB763 (blackface) - schematic read

Source: 55af493a-Fender_Vibroverb_Amp_AB763.pdf (1 page, 2047x1464 px bitmap scan; drawing code "C-FD"; patents 2817708, 2973681, 182859 and patents pending).
Notice: voltages read to ground with electronic voltmeter, values +/-20 %; all resistors 1/2 W 10 % unless specified; all capacitors at least 400 V unless specified.
Layout and topology are essentially the same as AA763; only the differences are called out in Vibroverb_revisions.md. Down-triangle = signal ground, right-triangle = chassis ground.
Channels labelled NORMAL and VIBRATO.

## 1. Tubes and heaters
| Position | Type | Function |
|---|---|---|
| Normal channel | 7025 | stage 1 + stage 2 |
| Vibrato channel | 7025 | stage 1 + stage 2 |
| "1/2 7025" x2 | 7025 | reverb recovery half + mixer half (cathode node E common) |
| PI | 12AT7 | long-tail phase inverter |
| Reverb driver | 12AT7 | halves paralleled, plate to TR4 |
| Vibrato | 12AX7 | oscillator + neon/photocell modulator |
| Output | 6L6GC x2 | push-pull, fixed bias |
| Rectifier | GZ34 | |
Heaters: 6.3 V "to all heaters and pilot light"; heater wiring not drawn. Mains: AC switch, 2 A fuse (SLO-BLO), 0.047 uF/600 V ground-switch cap, AC receptacle.
Export transformer table: TR1 125P50X / 125P5D, TR2 125C1A, TR3 125A7A, TR4 125A20B.

## 2. Preamp stages
### Normal channel
- Input jacks 1, 2: 68K per jack (read "68K, 68K") into grid node; jack-1 shorting contact has a resistor to ground (reads "1M" (?)).
- N1 (7025 sec. 1): plate load 100K to +410 V rail; plate printed +280 V (?); cathode 1500 ohm + 25 uF (cathode printed +2.0 V (?)).
- N2 (7025 sec. 2): plate load 100K to +410 V rail; cathode node A, +2.2 V printed, shared with Vibrato V2 (820 ohm + 25 uF); plate printed value unreadable (label cut by line). Plate coupling .047 uF then 220K to the mixer/mix node.
### Vibrato channel
- Same input network (68K x2, 1M grid leak).
- V1: plate 100K to +410 V, plate printed +220 V (?); cathode 1500 + 25 uF (printed +2.15 V).
- V2: plate load 100K to +410 V; plate printed +270 V (reads "+270V"); cathode 820 ohm + 25 uF, +2.2 V printed (tie A). Coupling: .02 uF.
- The .02 node feeds 500 pF to the reverb driver and the 10 pF || 1.5M (?, crop reads 1.5M; AA763 reads 4.7M) network into the mixer grid line.

## 3. Tone stack (same on both channels)
- 250 pF ("250PF") plate to treble pot top; 100K plate to stack node; .1 uF stack node to bass-pot top; .047 uF stack node to bass-pot bottom (both channels read .047); 6800 ohm bass bottom to ground.
- Pots (printed): TREBLE 250K-A (code "2-35"), BASS 250K-A, VOLUME 1M-A (code "2-35") - taper letter A, trailing code not interpretable.
- BRIGHT switch + 120 pF across volume pot on both channels.

## 4. Reverb and vibrato
- Reverb driver 12AT7: both halves in parallel; grid via 500 pF from the Vibrato V2 output node; 1M grid leak to ground; cathode 2200 ohm + 25 uF (printed +9.2 V (?)); plate +460 V (printed at the TR4 primary feed, node B).
- TR4: reverb transformer; no ratio or tank impedance printed. REVERB UNIT input/output shielded leads (dashed).
- Recovery (7025 half): grid from reverb output; 220K grid-to-ground; REVERB PEDAL switch grounds the grid when used; cathode 820 ohm + 25 uF (+2.1 V printed, node E); plate +270 V printed with 100K load to node D (+410 V).
- Plate -> .003 uF -> REVERB pot 100K-L -> ground; wiper -> 470K -> node (220K to ground) -> mixer grid line.
- Mixer (other 7025 half): grid fed by reverb return and by Vibrato V2 through 10 pF || 1.5M (?, crop reads 1.5M; AA763 reads 4.7M) (reads "3.3M"); cathode joined to recovery cathode (node E +2.1 V); plate printed +270 V (?); plate decoupling with 220K, .1 uF, 100K to +410 V (exact arrangement (?)); output .001 uF to the PI top grid. The Normal channel dry signal arrives through .047 uF + 220K at the mixer plate node.
- Vibrato (12AX7, opto tremolo): oscillator triode with 1M, 1M, 0.01 uF, 0.01 uF, 0.02 uF, 2.2M feedback network (read from scan, element-to-node assignment (?)); cathode 2700 ohm + 25 uF (+2.3 V printed); plate +260 V printed, plate load 220K to +460 V. SPEED pot 3M-RA ("1M-RA"/"3M-RA", read 3M-RA (?)) in series with 100K; vibrato pedal jack/switch.
- Modulator triode: plate +400 V printed, 10M (read "10M") from the plate node to +460 V, neon bulb in series with 100K to +460 V; cathode 100K + 25 uF (printed about +12 V (?)). Bulb and photoresistor in one ellipse. INTENSITY 50K-RA pot: top connects to the bias-supply node (the 10K-L bias pot line), bottom to ground, photocell lead to the wiper area; 27K from the bias node to ground. So this is bias modulation (photocell loads the output-tube bias line), not preamp-cathode modulation. Tie point A is only the shared cathode network.

## 5. Phase inverter (12AT7 long-tail)
- Input .001 uF to top grid; each grid 1M to node J. Bottom grid .1 uF (".1-200", 200 V) to left bus; left bus: 47 ohm to ground, 820 ohm NFB to the speaker secondary.
- Tail: 22K from left bus to J (J printed +116 V); 470 ohm from J to cathodes (cathodes +118 V printed).
- Plate loads: top 82K 5 %, bottom 100K 5 %; centre node C printed +450 V; plates printed +280 V (top) and +260 V (bottom).
- Plate outputs -> .1 uF each -> 1500 ohm grid stoppers -> 6L6 grids.

## 6. Output stage
- 2x 6L6GC, fixed bias; grid stoppers 1500 ohm each (printed "1500"); grid leaks 220K 5 % each to the bias line (bias printed -55 V at node between the two 220K).
- Screens: 470 ohm 1 W each from node B (+460 V); plate +460 V (printed at both plates and at the OT centre tap line +465 V).
- Cathodes direct to chassis ground. TR3 output transformer, one speaker + EXT SPKR jack; impedance not printed.
- Global NFB: 820 ohm from the speaker secondary hot side (EXT SPKR jack node) back to the PI left bus with 47 ohm to ground.
- Bias supply: TR1 tap -> 470 ohm 1 W -> diode (-/+ shown) -> 10K-L bias-adjust pot (marked with a circled "1") in series with 27K to ground; 25 uF/50 V electrolytic to ground.

## 7. Supply
- TR1 secondaries to GZ34: printed "355V AC" / "365V" (the second reads 365 V, probably a misprint for 355 V (?)).
- Rectifier output printed +465 V. STANDBY switch. First filter = two 70 uF/350 V electrolytics in series (labelled "70-350" each) each shunted by 220K 1 W equalising resistors. Choke TR2. Node B 20 uF/525 V (+460 V printed); B -> 1K -> node C 20 uF/525 V (+450 V at PI centre); C -> 4700 ohm -> node D 20 uF/525 V (+410 V rail).
- Printed: B +460, C +450, D +410; rectifier +465.

## 8. Not legible / uncertain
- Normal V1 / V2 plate voltages; jack-1 resistor value; exact reverb-mixer plate decoupling; the "2-35" pot codes.
- SPEED pot value (3M vs 1M); 10M vs 1M in modulator; neon/photocell wiring; vibrato 56K/100K cathode resistor on 12AX7 second triode (100K read).
- Transformer ratios, tank impedance, speaker impedance.
