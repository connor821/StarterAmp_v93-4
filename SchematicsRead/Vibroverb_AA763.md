# Fender Vibroverb Amp AA763 - schematic read

Source: ce914c87-Fender_Vibroverb_Amp_AA763.pdf (1 page, 2064x1498 px, bitmap scan; drawing code "K-FC"; Fender Electric Instrument Company, Fullerton).
Patents on drawing: 2817708, 2973681, 182859 (read), "patents pending".
Notice on drawing: voltages read to ground with electronic voltmeter, values shown +/-20 %; all resistors 1/2 W 10 % unless specified; all caps at least 400 V unless specified.
Ground symbols: down-pointing triangle = signal ground, right-pointing triangle = chassis ground (6L6 cathodes, heater return). Both are ground for modelling.
The two input channels are labelled NORMAL and VIBRATO (the tremolo channel).

## 1. Tubes and heaters
| Position | Type printed | Function |
|---|---|---|
| NORMAL | 7025 | stage 1 + stage 2 of the Normal channel |
| VIBRATO | 7025 | stage 1 + stage 2 of the Vibrato channel |
| "1/2 7025" x2 | 7025 | one 7025 envelope: reverb recovery half + mixer half (joined cathode node E) |
| PI | 12AT7 | phase inverter (long-tail) |
| reverb driver | 12AT7 | both halves paralleled, plate to reverb transformer TR4 |
| vibrato | 12AX7 | oscillator + modulator that drives a neon lamp / photocell pair |
| outputs | 6L6GC x2 | push-pull |
| rectifier | GZ34 | |
Heaters: "TO ALL 6.3V HEATERS AND PILOT LIGHT" from TR1; wiring not drawn. TR1 has a 2 A fuse (marked SLO-BLO), AC switch, AC receptacle, ground switch with 0.047 uF/600 V cap to ground.
Export transformer numbers listed: TR1 125P5DX / 125P5D, TR2 125C1A, TR3 125A7A, TR4 125A20B.

## 2. Preamp stages
### Normal channel
- Input: jack 2 -> 68K -> grid node; jack 1 -> 68K -> grid node (read "68K, 68K"); a small resistor (read "1M") from the jack 1 shorting contact to ground. Value and exact placement of the grid leak (?).
- N1 (7025 section 1): plate load 100K to the +410 V rail; plate printed +280 V (?, could be +260/+230); cathode 1500 ohm + 25 uF; cathode printed +2.0 V (?).
- Tone stack (section 3) from N1 plate.
- N2 (7025 section 2): plate load 100K to +410 V rail; plate printed "+18?V" (?); cathode to tie point A (+2.2 V printed), common with the Vibrato channel's second stage cathode. Output coupling .047 uF then 220K to the mixer plate/PI mix node.
### Vibrato channel
- Input network identical (68K, 68K, grid leak).
- V1 (7025 section 1): plate 100K load to +410 V; plate +210 V printed; cathode 1500 + 25 uF, +2.15 V printed.
- V2 (7025 section 2): plate load 100K to +410 V; plate +270 V (?) printed; cathode 820 ohm + 25 uF with cathode printed +2.2 V; tie point A to the other channel. Output .02 uF coupling.
- V2 output node feeds (a) 500 pF to the reverb driver grid (b) a 10 pF in parallel with 4.7M (?) into the mixer grid line (see section 4).

## 3. Tone stack (identical topology both channels)
- 250 pF (printed "250PF"/"230PF") from plate to top of treble pot; 100K from plate to stack node; 0.1 uF (".1") from stack node to bass pot top; 0.05 uF (printed ".05"/".08S", normal channel; could be .047) and 0.033 uF (printed ".033", vibrato channel) from stack node to bass pot bottom; 6800 ohm bass-pot bottom to ground.
  Note: the bass-side cap differs between the two channels as read: normal .05(?), vibrato .033 (?).
- Pots: TREBLE 250K-A (plus a code that reads "2-35", not interpretable), BASS 250K-A (?), VOLUME 1M-A (code "2-35"?). Taper letters read A.
- BRIGHT switch (drawn as switch + cap): 120 pF across volume pot (top to wiper) on both channels.
- Volume wiper -> N2 / V2 grid with no visible grid stopper.

## 4. Reverb and vibrato
### Reverb driver (12AT7, both halves parallel)
- Grid fed from vibrato-channel V2 output through 500 pF; 1M grid leak to ground.
- Cathode 2200 ohm + 25 uF; printed cathode voltage +9.2 V (?).
- Plate to TR4 primary, B+ node B (printed +460 V).
- TR4 secondary -> REVERB UNIT INPUT. Transformer and tank impedances are not printed.
### Reverb recovery (7025 half)
- Reverb unit OUTPUT -> grid; 220K from grid to ground; REVERB PEDAL switch (dashed) shorts recovery grid to ground.
- Cathode 820 ohm + 25 uF (cathode +2.1 V printed, node E tied to the mixer cathode). Plate load 100K to node D (+410 V); plate printed +270 V.
- Plate -> 0.003 uF -> REVERB pot 100K-L (L = linear taper in Fender notation) to ground; wiper -> 470K -> node with 220K to ground; that node is the reverb return line that goes to the mixer-grid line. 
### Mixer (other 7025 half)
- Grid line: reverb return enters at the top; vibrato V2 output enters through the 10 pF/4.7M (?) network, i.e. dry vibrato channel is heavily attenuated into the mixer grid. The 10 pF || 4.7M reading is less than certain.
- Cathode joined with the recovery cathode (node E, +2.1 V).
- Plate load/decoupling: 220K plus a 0.1 uF cap and 100K to the +410 V rail (exact topology (?)). Plate printed +270 V (?). Mixer plate is the mixing node where the Normal channel's .047 uF/220K output also arrives; from there .001 uF couples to the phase-inverter grid.
### Vibrato (neon-lamp / photocell tremolo; 12AX7)
- Oscillator triode: phase-shift/feedback network of 1M, 1M, 0.01 uF, 0.01 uF (read ".01, .01") and 0.02 uF with 2.2M; cathode 2700 ohm + 25 uF (+2.3 V printed); plate +260 V printed, plate load 220K to +460 V (node B).
- SPEED: 3M-RA variable resistor in series with 100K to ground; vibrato pedal jack/switch (dashed) in the same circuit.
- Second triode: plate +400 V printed through 10M (?) to +460 V and feeds a neon bulb; cathode 56K (read "86K"/"56K") + 25 uF, +5.6 V (?); 100K from the lamp's other side to +460 V. The lamp and a photoresistor are drawn in one ellipse (opto-coupler).
- INTENSITY pot 50K-RA: its top end goes up to the bias-supply node (the 10K-L bias-adjust pot / 27K line), its bottom to ground, and the photocell is connected from the pot wiper area to the same node. The 27K from the bias node to ground is part of this network. This is bias-modulation (the photocell varies the shunt on the output-tube bias line); the boxed tie point A is only the shared V1B/V2B cathode network and has nothing to do with vibrato here. Photocell/pot connection detail (?).

## 5. Phase inverter (12AT7, long-tail)
- Signal: mixer node -> .001 uF (printed ".001") -> top-triode grid; 1M grid leak each grid to node J.
- Bottom grid: .1 uF/200 V (".1-200") to left bus; left bus: 47 ohm to ground; 820 ohm to the speaker secondary (global NFB).
- Tail: 27K (printed "27K", the AB763 equivalent reads 22K - digit ambiguous (?)) from left bus to node J (J printed +116 V); 470 ohm from J to the common cathodes (cathodes printed +118 V). Check: ~4-5 mA total tail current.
- Plate loads: 100K 5 % (top) and 100K 5 % (bottom) from the centre node C (printed +450 V) to the plates. Plates printed +280 V (top) and +260 V (bottom) (digits (?)).
- Plate outputs -> .1 uF each -> 6L6 grids.

## 6. Output stage
- 2x 6L6GC, fixed bias, push-pull, cathodes to chassis ground.
- Grid network: 0.1 uF couplers; 220K 5 % from each grid to the bias node (the two 220K meet at the bias line); printed bias -55 V.
- Screens: 470 ohm 1 W each from node B (+460 V printed). Plates +460 V (printed on both plates, and +465 V printed at the OT centre-tap line).
- Output transformer TR3: single speaker plus EXT SPKR jack; speaker impedance not printed.
- Global NFB: 820 ohm from the speaker/secondary hot (the EXT SPKR jack node) to the PI left bus (junction with the 47 ohm to ground and the 27K).
- Bias supply: TR1 tap -> 470 ohm 1 W -> diode (printed "-" left, "+" right) -> 10K-L pot (bias adjust, wiper to bias line) in series with 27K to ground; 25 uF/50 V electrolytic from diode output to ground.

## 7. Supply
- TR1 with 355 V - 0 - 355 V (printed "355V AC") to GZ34. Rectifier output printed +465 V.
- STANDBY switch, then 2 x 20 uF/525 V first caps (both listed "20-525"), choke TR2, then B node (+460 V printed) 20 uF/525 V.
- B -> 1K -> node C 20 uF/525 V (PI centre, +450 V printed) -> 4700 ohm -> node D 20 uF/525 V (+410 V printed rail, preamp).
- Note the "1K" and "4700" order as drawn from left: D, 4700, C, 1K, B.
- Printed nodes: B +460, C +450, D +410; GZ34 output +465.
- Heater winding: 6.3 V (not otherwise labelled).

## 8. Not legible / uncertain
- Tone pot "2-35" codes; bass-cap values (.05 vs .047, .033).
- Normal V1 / V2 plate voltages; vibrato V1 grid-leak "1M"; jack-1 resistor value.
- Reverb pot taper letter, 10 pF/4.7M network in the mixer grid line, mixer plate decoupling.
- Photocell/neon wiring detail, 10M/100K, 56K, 2.2M values.
- Transformer and tank impedances; speaker impedance.
