# Fender Twin Reverb-Amp AB763 (blackface) - schematic read

Source: `bb76ace4-Fender_Twin_Reverb_AB763.pdf` (2 pages, 1-bit scans: p1 schematic 1994x1463, p2 chassis layout 2259x1384). Header text: "Fender Musical Instruments, a division of Columbia Records Distribution Corp., Fullerton, California", patent line, code "C-FD".
Sheet notes (printed): 1) voltages read to ground with electronic voltmeter, values +/-20 %; 2) all resistors 1/2 W 10 % unless specified; 3) all capacitors at least 400 V rating unless specified.
Conventions: Fender "1500" = 1.5k, "4700" = 4.7k, "820" = 820 ohm, "470-1W" = 470 ohm 1 W. Caps in uF unless "PF". Pot tags like "2-35" are Fender pot codes (not values). "(?)" = not certain. "(computed)" = my arithmetic from printed values, not printed.

## 1. Tube complement and heater notes
| Ref (as drawn) | Tube | Function |
|---|---|---|
| Normal V1A / V1B | 7025 | Normal channel gain stage 1 / stage 2 |
| Vibrato V2A / V2B | 7025 | Vibrato channel gain stage 1 / stage 2 |
| "1/2 7025" (upper, +280 V) | 7025 half | Mixer stage (called V4A on the AA270) |
| "1/2 7025" (lower, +275 V) | 7025 half | Reverb recovery stage (V4B on AA270) |
| 12AT7 (upper) | 12AT7 | Phase inverter, long-tail |
| 12AT7 (lower left) | 12AT7 | Reverb driver (two triodes paralleled) |
| 12AX7 | 12AX7 | Vibrato oscillator (left triode) + modulator (right triode) |
| 6L6GC (4) | 6L6GC x4 | Power amp, two pairs in parallel push-pull |
| (none) | solid state | Rectifier: 6 diodes, 3 in series per leg of a centre-tapped winding |
- Heaters: "TO ALL 6.3 V HEATERS & PILOT LIGHT" from a TR1 winding, with an arrow labelled "INTERNAL GROUND" (heater centre tap to chassis ground). No hum-balance resistors drawn (the AA270 draws two 100 ohm).
- Power-up: AC switch, 2 1/2 A slo-blo fuse, ground switch with .047 uF/600 V cap, AC receptacle with a neon lamp on the mains.

## 2. Stage by stage
### Normal channel
- **Input network**: two jacks (1 and 2). Two 68k resistors in series; the V1A grid is the junction of the two 68k; 1M from the jack-1 end to ground. Jack-switch contacts exist but their routing is not traceable on this scan (see section 8). Convention only (not read): jack 1 = full gain, jack 2 = about 6 dB down.
- **V1A (7025, first gain stage)**: plate load 100k (to the +410 V rail); plate +260 V; cathode 1500 || 25 uF, cathode +2.0 V.
- **Coupling**: V1A plate goes directly (no series cap) to the top of the tone stack (100k slope resistor + 250 pF); the stack output goes to the Volume pot.
- **Volume**: 1M, "A" audio taper ("1M-A 2-35"). Bright: 120 pF from the Volume-pot top to the wiper through a switch ("BRIGHT").
- **V1B (7025, second gain stage)**: grid at the Volume wiper (no grid-stopper drawn). Plate load 100k to +410 V; plate +270 V. Cathode goes to node "A" = 820 || 25 uF shared with Vibrato V2B (cathode +2.0 V).
- **Output**: V1B plate -> .047 uF -> 220k (series mixing resistor) -> node N1 = phase-inverter input node (N1 -> .001 uF -> PI top grid).

### Vibrato channel
- Input network identical to Normal (68k, 68k, 1M).
- **V2A**: plate load 100k to +410 V; plate +250 V; cathode 1500 || 25 uF (+2.0 V).
- Tone stack and Volume identical to Normal; Middle is a 10k-A pot; Volume "1M-A 2-35"; bright 120 pF.
- **V2B**: plate load 100k to +410 V; plate +240 V; cathode node "A" 820 || 25 uF (+2.0 V) shared with V1B.
- **Output**: V2B plate -> .02 uF -> node VO. VO goes (a) through **3.3M || 10 pF** to the grid node G of the mixer half-7025 (+280 V stage), and (b) through a **500 pF** cap to the reverb-driver 12AT7 grids. The reverb send is taken from the vibrato channel only; the Normal channel has no reverb and no vibrato.

### Mixer half-7025 (upper "1/2 7025", +280 V)
- Grid node G: vibrato-channel signal arrives from VO through 3.3M || 10 pF; the reverb return (node R, junction of 470k / 220k-to-ground, see section 4) is wired straight to G (no series resistor between R and G read).
- Plate +280 V. Plate load: 100k from the +410 V rail. Cathode +2.0 V on node "E" (820 || 25 uF, shared with the reverb recovery half).
- Output: mixer plate -> .1 uF -> node N2 -> 220k -> node N1 (PI input). The Normal channel (.047 uF + 220k) feeds the same N1. So two 220k resistors (Normal; vibrato/reverb mix) sum at N1; N1 -> .001 uF -> PI top grid. N2 is also the top of the INTENSITY pot (optical vibrato shunt, section 4).

## 3. Tone stack (identical in both channels)
Standard Fender passive TMB (values read on both channels):
- Plate node -> **100k** slope resistor -> common node. Plate node -> **250 pF** -> Treble pot top.
- Treble 250k-A ("2-35"): wiper = stack output to Volume; bottom to Bass pot top.
- **.1 uF** from the slope-resistor node to the Treble-bottom / Bass-top node.
- Bass 250k-A (wired as a variable resistor to the next node).
- **.047 uF** from the slope-resistor node to the Bass-bottom / Middle-top node.
- Middle 10k-A to ground.
- Volume 1M-A; bright cap 120 pF across Volume top to wiper with a switch.
- No shift or boost switching. Pot tapers: all "A" (audio); Reverb 100k-L (linear); Speed 3M-RA; Intensity 50k-RA.

## 4. Reverb and vibrato
**Reverb driver (12AT7, both halves paralleled)**: grid input 500 pF from the vibrato output node; grid leak 1M to ground. Cathode 2200 || 25 uF, cathode +8.6 V (printed +8.6V). Plate to reverb transformer TR4 primary: plate printed +440 V; other end of TR4 primary to node B (+458 V). TR4 secondary drives the pan input.
**Reverb pan**: 4-spring unit (input/output RCA) with "REVERB PEDAL" footswitch (shorting switch to ground).
**Recovery half-7025 (lower)**: grid from the pan output with a **220k** to ground. Plate +275 V, plate load 100k from +410 V (node D). Cathode 820 || 25 uF, +2.0 V (node E).
**Return path**: plate -> .003 uF -> **Reverb pot 100k-L** ("REVERB") -> wiper -> **470k** series, **220k** to ground, then to the mixer-stage grid node.
**Vibrato (12AX7: oscillator left triode, modulator right triode; neon lamp + photocell "bulb")**:
- Oscillator triode: plate load **220k** to B (+458 V); plate **+300 V**; cathode **2700 || 25 uF**, +2.0 V. Plate -> **.02 uF** -> node P1. P1 -> **.01 uF** -> node P2. P2 -> **.01 uF (label reads ".0?", (?))** -> node P3 = oscillator grid. P2 -> **1M** -> node NB; P3 -> **1M** -> node NB. NB -> **2.2M** -> the raw negative bias node (diode side, see section 7). P1 -> **SPEED 3M-RA** (variable R) -> **100k** -> ground. NB also goes to the VIBRATO PEDAL jack (dashed pedal box, shorting/jack arrangement; exact switching not resolved).
- Modulator triode: grid fed from node P2 (long wire). Plate **+380 V**, plate load **10M** to B (+458 V); cathode **+17.0 V**, **100k || 25 uF**.
- Lamp: modulator plate -> neon lamp -> **100k** -> B (+458 V) (lamp in series with 100k, in parallel with the 10M). Photocell (other element in the "bulb") one end to ground; other end to the **INTENSITY 50k-RA** pot wiper; pot ends: top to node N2 (the 220k / .1 uF mixing node, section 2) and bottom to ground. So the optical element shunts the vibrato-channel signal at N2 (tremolo/vibrato acts on the vibrato channel path only, not on the Normal channel).
- Printed: B+ for this tube is +458 V (node B).

## 5. Phase inverter: 12AT7 long-tail pair (cathode-coupled), grids self-referenced to the tail
- Top grid: .001 uF from the mixing node; **1M** from top grid to the "+98 V" node. Bottom grid: **.1 uF / 200 V** to the NFB node (junction of 22k and 100 ohm); **1M** from bottom grid to the "+98 V" node.
- Cathodes joined (+100 V printed) -> **470 ohm** -> "+98 V" node -> **22k** -> NFB node -> **100 ohm** -> ground. Total tail about 22.6k (computed). DC check: ~4.5 mA total at +100 V (computed) is consistent with the plate loads.
- Plate loads (5 %): **82k** top triode, **100k** bottom triode, both from node C (+450 V). Plates printed +245 V (top) and +235 V (bottom).
- Outputs: each plate through **.1 uF** to the grid node of one phase (two 6L6GC per phase).

## 6. Output stage
- 4 x 6L6GC, two per phase in parallel. Cathodes straight to ground (no cathode resistor, no bypass).
- Bias: **fixed, -52 V** printed on the grid-return line (see supply). Each phase node has **220k 5 %** to the -52 V line.
- Grid stoppers: **1500 ohm in every grid lead** (4 total).
- Screen resistors: **470 ohm 1 W** per tube, from the +458 V node (B). Screens printed +458 V.
- Plates printed **+460 V**; output transformer primary centre tap +460 V.
- Output transformer TR3 (export table lists "TR3-125A29A"); the secondary primary impedance is **not printed**. Two speakers drawn plus an EXT SPKR jack; speaker impedance/size **not printed** on this schematic.
- **Global NFB**: **820 ohm** from the output-transformer secondary "hot" (the EXT SPKR jack node) to the NFB node of the PI (22k / 100 ohm junction, which is the AC ground for the PI bottom grid via .1 uF). Divider 100/(820+100) = 0.109, about -19 dB (computed, ignores the 22k). Return point: PI side, not the input stage. **No presence control.**
- Standby switch is in the B+ line after the rectifier/reservoir (before TR2 choke).

## 7. Supply
- Power transformer TR1 (export table: TR1-125P34A / 125P34AX): HV secondary printed **340 V AC** at each end of a centre-tapped winding (centre tap to ground); heater winding 6.3 V centre-tapped to chassis ("INTERNAL GROUND"); an additional low-voltage tap on the HV winding (between the 340 V AC+ end and the grounded centre tap; layout page labels a wire "48 VAC") feeds the bias rectifier.
- Rectifier: **solid-state, 6 diodes** (3 in series per leg, one leg per 340 V AC end), no rectifier tube; both legs join at the + rail.
- Reservoir: **2 x 70 uF / 350 V in series** (35 uF effective, computed), each with a **220k 1 W** balance resistor. Rail after rectifier printed **+460 V**.
- **Standby switch** in the + rail after the reservoir. The node after the switch (+460 V) feeds the plates directly (output-transformer centre tap and plate leads) and, through the choke **TR2**, node **B**.
- B (+458 V; 6L6 screens via 470 ohm 1 W each, vibrato tube, reverb TR4) has **20 uF / 525 V**.
- B -> **1k 1 W** -> node **C (+450 V)** with **20 uF / 525 V** (PI plate loads).
- C -> **4700 ohm 1 W** -> node **D (+410 V)** with **20 uF / 525 V** (preamp 100k plate loads, mixer, reverb recovery).
- **Bias supply**: tap (via **470 ohm 1 W**) -> diode (conducting so that the node goes negative; "-" printed at the node) -> raw negative node. Raw node has a **50 uF** electrolytic to ground (printed "50 / 50"; polarity + to ground) and feeds the **10k bias pot** (one end to the raw node, other end through **27k** to ground, wiper -> **-52 V** line). The raw node also feeds the vibrato oscillator 2.2M. Raw node voltage not printed. Pot range not printed.
- Printed B+ list: +460 V (rail after standby; plates; OT centre tap); +458 V (B: screens, vibrato, TR4); +450 V (C: PI); +410 V (D: preamp); +440 V (reverb driver plate, after TR4 primary drop); +380 V and +300 V (vibrato modulator / oscillator plates).
- **Sag sources** (from the circuit): solid-state rectifier (no tube drop), so sag is set by transformer regulation and the 35 uF reservoir; plates are fed ahead of the choke and the 1k/4.7k RC chain, so the 1k and 4.7k and the choke affect only screens and preamp; 4 x 6L6GC draw heavy current. Choke DCR / transformer resistance not printed.

## 8. Not legible / uncertain
- Jack-switch routing of the two input jacks (drawn with a curved shorting contact from jack 2 to the jack-1 end; which 68k is in circuit per jack is not resolved).
- Vibrato oscillator: the cap from P2 to P3 reads ".0?" (probably .01); VIBRATO PEDAL jack switching; neon/photocell part numbers and the lamp/cell characteristics.
- Bias filter cap: printed "50 / 50" (read as 50 uF; voltage rating not printed (?)); raw bias-supply voltage not printed; whether the 10k pot wiper is tied to one end (drawn with a loop) is not certain.
- Output transformer primary impedance, secondary tap impedances, speaker impedances: not printed. (OT part "TR3-125A29A" and TR1/TR2/TR4 numbers from the export table; AB763 export table: TR1-125P34AX, TR1-125P34A, TR2-125C1A, TR3-125A29A, TR4-125A20B.)
- Choke inductance/DCR, transformer voltages other than the printed 340 V AC (and the layout-page '48 VAC' label on the bias tap wire).
- Layout page (p2) wire colours/positions were not transcribed beyond what is noted.
