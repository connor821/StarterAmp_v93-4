# Fender Deluxe Reverb-Amp AB763 (blackface) - schematic read

Source files: `a3675550-Fender_deluxe_reverb_ab763.pdf` and `23ad1c12-Fender_deluxe_reverb_ab763_1.pdf`. **Byte-identical** (same md5 471b27ba...), so one read covers both. 2 pages: p1 = schematic (1-bit scan, title "FENDER MODEL DELUXE REVERB-AMP AB763 SCHEMATIC", header "Fender Electric Instrument Company, Fullerton, California"), p2 = chassis layout/wiring (used only to cross-check pot values and tube order).
Notes printed on the sheet: voltages read to ground with electronic voltmeter, values +/-20 %; all resistors 1/2 W 10 % unless specified; all capacitors at least 400 V rating unless specified. Patent line: circuit patent #2817708, #2973681, design patent #192859.
Convention: Fender "1500" = 1.5k, "6800" = 6.8k, "2200" = 2.2k, "820" = 820 ohm; "470-1W" = 470 ohm 1 W; caps in uF unless "PF". Pot suffix "2-35" is a Fender pot-code tag, not a value. Pot letter "A" = audio (log) taper, "L" = linear, "RA" = reverse audio.

## 1. Tube complement and heaters

| Ref (as printed on schematic) | Tube | Function |
|---|---|---|
| V1 | 7025 (12AX7 low-noise) | Normal channel: V1A (first gain), V1B (second gain) |
| V2 | 7025 | Vibrato channel: V2A, V2B |
| V3 | 12AT7 | Reverb driver, both triodes paralleled |
| V4 | 7025 | V4A (upper symbol on sheet) = reverb recovery/mixer? **see section 4 naming note**; the two halves are the reverb mixer/gain stage and the reverb recovery stage |
| V5 | 12AX7 (label "12AX7") | Vibrato oscillator (V5A) + modulator/lamp driver (V5B) |
| V6 | 12AT7 | Phase inverter, long-tailed pair |
| V7, V8 | 6V6GT | Push-pull power output (fixed bias) |
| V9 | GZ34 | Rectifier (tube, full-wave) |

- Layout page (p2) tube row, left to right: GZ34, 6V6GT, 6V6GT, 12AT7, 12AX7, 7025, 12AT7, 7025, 7025 - agrees with the schematic.
- Heaters: 6.3 V winding with the centre tap grounded (centre-tap arrow shown), "TO ALL 6.3V HEATERS AND PILOT LIGHT". No hum-balance resistors on this sheet. Rectifier filament from separate winding; two legs printed "330 VAC" (each side of HV winding) to GZ34 plates.
- Mains: 120 V 60 Hz, grounded cord via a ground switch (+ .047 uF 600 V cap), AC switch and 1 A slo-blo fuse in series with the TR1 primary. (Note: the sheet says "1 AMP FUSE" - later revisions use 2 A.)

## 2. Stage by stage

### Normal channel (V1, 7025)
- **Input network.** Jack 1 and jack 2. Each jack goes through its own 68k to the common V1A grid node (jack 1 -> 68k -> grid; jack 2 -> 68k -> grid), so jack 2 is the pad input. 1M from the jack-1 input node to ground (shorting-contact switching as drawn). No grid stopper drawn.
- **V1A**: plate printed +180 V; plate load 100k to the common supply node (see section 8, node "D" side); cathode 1500 ohm || 25 uF, cathode +1.3 V.
- **Coupling to tone stack**: none in series - plate node goes straight to the stack's 250 pF and 100k; the stack's own caps block DC.
- **Tone stack**: see section 3.
- **Volume**: 1M-A ("1M-A 2-35"), audio taper, wiper to V1B grid. **No bright cap on the Normal channel.**
- **V1B**: plate +180 V; plate load 100k; cathode returns to node "A" (shared with V2B: 820 ohm || 25 uF), +1.3 V.
- **Output**: V1B plate -> .047 uF -> 220k -> mixing node "M" (this node = reverb-mixer plate node -> .001 uF -> PI grid). Dry Normal path therefore **bypasses the reverb mixer grid stage and the tremolo**.

### Vibrato channel (V2, 7025)
- Input network identical to Normal (68k / 68k, 1M to ground).
- **V2A**: plate +170 V, plate load 100k; cathode 1500 ohm || 25 uF (+1.3 V).
- Tone stack identical (section 3).
- **Volume**: 1M-A ("1M-A 2-35"). **Bright cap 47 pF** across the volume pot from the treble wiper (top of pot) to the V2B grid (wiper) - a "bright" capacitor on the Vibrato channel only.
- **V2B**: plate +170 V, plate load 100k; cathode 820 ohm || 25 uF = node "A", +1.3 V (also V1B cathode).
- **Output**: V2B plate -> .02 uF -> V4 mixer grid node (the node also feeds the 500 pF reverb-send cap, see 4).

The preamp plate loads are each 100k to a shared supply node; the V1B and V2B plate nodes are **not** tied together on this sheet (each has its own 100k, V1B -> .047/220k route, V2B -> .02 route).

## 3. Tone stack (both channels identical; standard Fender TMB without mid)
- Plate node -> 250 pF -> top of TREBLE pot (250k-A).
- Plate node -> 100k -> stack node; stack node -> .1 uF -> bass-pot top (= treble-pot bottom); stack node -> .047 uF -> bass-pot bottom.
- TREBLE 250k-A (2-35), wiper = stack output -> Volume.
- BASS 250k-A (2-35), wired as a variable resistor (wiper tied to one end), bottom end -> **6800 ohm to ground**.
- VOLUME 1M-A.
- Bright cap: 47 pF on the Vibrato channel only (see 2).
- No mid control, no switching.

## 4. Reverb
**Naming note:** the schematic prints "V4" next to a single 7025 symbol for two half-tubes; the 1/2 7025 whose grid receives the dry+reverb mix is drawn upper-centre, the recovery half lower. Roles by circuit trace (not by label):

- **Driver V3, 12AT7, both halves paralleled** (grids, plates, cathodes tied). Grid: 500 pF coupling (from the V2B-output / V4-mixer grid node), grid leak 1M to ground. Cathodes tied: 2200 ohm || 25 uF, +8.7 V. Plate printed +410 V, loading the primary of **TR4** (reverb transformer, export P/N table lists TR4-125A20B), other primary end to supply node "B". TR4 secondary drives the reverb pan input (RCA, "REVERB UNIT", 4-spring).
- **Recovery half (lower 1/2 7025)**: grid from pan output, **220k grid leak to ground**; plate +170 V; plate load 100k (printed "100K", wattage not read) to node "D"; cathode 820 ohm || 25 uF = node "E" (+1.3 V). Plate -> **.003 uF** -> REVERB pot 100K-L (linear, wiper). Footswitch ("REVERB PEDAL") shorts the recovery-stage grid to ground (dashed shielded cable) = reverb off.
- **Wet mix**: REVERB pot wiper -> **470k** -> mixer-grid node (this node also has **220k to ground**).
- **Dry/channel mix at mixer grid (upper 1/2 7025)**: V2B -> .02 uF -> node B; node B -> (**3.3M || 10 pF**) -> mixer grid node. So the Vibrato channel enters the mixer grid through 3.3M||10 pF; wet reverb enters via 470k; 220k grid leak.
- **Mixer stage (upper 1/2 7025)**: plate +180 V; plate load 100k to D; cathode node "E" (820 ohm || 25 uF, +1.3 V, shared with the recovery half). Plate -> **.1 uF** -> node P; P -> **220k** -> node M. Node P also goes to the tremolo opto-resistor (section 5).
- **Node M**: V1B via .047 uF + 220k joins here; M -> .001 uF -> V6A grid (PI input).
- Not present on AB763: 2000 pF across the 220k, push-pull "reverb on" switch, 1200 pF output grid caps (these appear on later revisions).

## 5. Tremolo (called "Vibrato" on the panel)
**V5 12AX7**, two sections:
- **V5A oscillator**: plate printed +270 V, plate load 220k to supply; cathode 2700 ohm || 25 uF, +2.1 V.
- **V5B modulator/lamp driver**: plate printed +345 V (node "B"), cathode 100k || 25 uF, cathode printed +17.0 V. Plate supply path: 10M and a series neon-lamp + 100k combination to node "B" (the lamp-plus-photocell opto assembly is drawn as a neon bulb with a resistor element; the photoresistor side goes to the 50K-RA INTENSITY pot). Exact lamp/10M topology: (?) - values read: 10M, 100k, neon.
- **Phase-shift network values read**: 1M, 1M, 2.2M, .01, .01, .02, 3M-RA SPEED pot in series with 100k to ground (speed range controlled by the 3M reverse-audio pot; the 68'-manual statement that speed control sits in the first leg of the phase-shift network is consistent). Complete node-by-node topology of the three-stage network: (?) - only values are certain.
- **Intensity**: 50K-RA pot. Photoresistor from node P (reverb mixer plate/220k junction) through the pot to ground = shunts the signal ahead of the 220k to node M, giving amplitude modulation of the Vibrato channel path only.
- **Bias supply is shared on the same corner of the sheet** (see 7): the "10K-L" with "10K" pot cluster next to INTENSITY is the bias adjust, not tremolo.
- **Vibrato pedal jack** (switch to ground) connected at the oscillator input network next to the 1M/2.2M resistors (kills oscillation).

## 6. Phase inverter V6, 12AT7, long-tailed pair (self-balancing is not used; fixed LTP)
- V6A grid: .001 uF from node M; 1M from grid to the "+75.5 V" cathode-bias node.
- V6B grid: **.1 uF - 200 V** (printed ".1-200") from the NFB node; 1M from grid to the +75.5 V node.
- Common cathodes +77 V: via 470 ohm to the +75.5 V node; +75.5 V node via **22k** to the NFB node, which goes to ground through **47 ohm**.
- NFB return resistor **820 ohm** from the output-transformer secondary (speaker line) to the NFB node (47 ohm to ground): feedback ~ 47/(820+47) = 5.4 % (-25.3 dB).
- Plate loads (5 % parts): V6A **82k 5 %** (plate +170 V), V6B **100k 5 %** (plate +180 V), both fed from node "C" (printed +325 V) at the junction between the two resistors.
- Output coupling: .1 uF to each 6V6 grid (".1", 400 V min).

## 7. Output stage
- **Tubes**: 2 x 6V6GT, push-pull class AB, **fixed bias**; cathodes grounded.
- Plate voltage printed **+415 V** (both plates); screen supply line printed **+415 V** via **470 ohm 1 W** per screen (from the screen line); output-transformer centre tap printed **+420 V**.
- **Grid stoppers** 1500 ohm each; **grid-leak resistors 220k 5 %** each, returned to the bias supply (junction of the two 220k = -35 V line).
- **Bias**: negative supply printed **-35 V**. Source: HV winding AC (one "330 VAC" lead) -> **470 ohm 1 W** -> diode (marked "-"/"+" as drawn; half-wave) -> filter **25 uF / 50 V** (printed "25 / 50", polarity: - to the pot node) -> bias-adjust pot **10K-L** whose end goes to ground through **10k**; wiper = -35 V line (via the two 220k leaks). So the pot, plus the bottom 10k, divides the negative rail; adjust sets 6V6 idle current.
- **Output transformer**: TR3 (P/N 125A1A in the export table). Primary impedance **not printed**. 8 ohm speaker symbol; EXT SPKR jack in parallel (switching jack disconnects internal speaker). Speaker impedance printed: 8 ohm not legible on this sheet - the speaker symbol carries no value on AB763 (?).
- **No power-tube cathode resistor**, no 1200 pF grid caps, no screen stoppers other than the 470 ohm 1 W (these also carry screen current - effectively a plate-supply decoupling).
- **NFB**: 820 ohm from speaker/secondary to the PI NFB node (47 ohm to ground, 22k to cathode bias node, .1 uF to V6B grid).

## 8. Supply
- Rectifier GZ34, full wave, 330 VAC per side printed.
- Filter chain: rectifier -> STANDBY switch -> two **16 uF 450 V** electrolytics in parallel (first reservoir) -> choke **TR2** (P/N 125C3A) -> node **B** (**16 uF 450 V**) -> **10k** -> node **C** (16 uF 450 V) -> **10k** -> node **D** (16 uF 450 V). (Both series dropping resistors are 10k on AB763; the later AB868/boost have 10k then 2.2k.)
- Printed voltages: +420 V at OT centre tap and screens/plates +415 V (reservoir/choke node); V3 plate +410 V (node B); node C +325 V (PI plate supply); nodes D / preamp plates +170-180 V (V1A +180, V1B +180, V2A +170, V2B +170, mixer +180, recovery +170, V6A +170, V6B +180).
- Oscillator: +270 V and +345 V printed at the V5 plates; +345 V is at node "B"-derived plate supply (higher than node C, so it is taken from before the C node).
- Bias: -35 V. Heater: 6.3 V, centre tap ground.

## 9. Not legible / uncertain
- Wattages of the preamp plate loads (printed only "100K"); on later sheets they are 1 W.
- Oscillator/lamp (V5) exact node connectivity: values legible, topology only partly traceable.
- Speaker impedance value and OT primary Z not printed on this sheet.
- Bias diode orientation: schematic shows polarity marks "-" on the pot node and "+" on the 470 ohm side; net result is a negative -35 V supply as printed.
- Node D exact supply (D labelled, no voltage printed at D itself).
- Page 2 is a layout only; no additional values. It prints the speed pot as "3.0M RA" and shows "56K" in the oscillator corner (looks like a layout-only label; not used).
