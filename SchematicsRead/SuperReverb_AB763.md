# Fender Super-Reverb-Amp AB763 (blackface) - schematic read

Source: `60a870db-fender_super_reverb_ab763.pdf` (scanned schematic plus a photographed copy and a layout page; header "Fender Electric Instrument Co., Fullerton, California", "C-FD"). Printed notices: "Voltages read to ground with electronic voltmeter, values shown +/-20 %"; "All resistors 1/2 watt 10 % tolerance if not specified"; "All capacitors at least 400 volt rating if not specified". 2 amp slo-blo fuse.
Conventions: "1500" = 1.5k; "470-1W" = 470 ohm 1 W; "25-25" = 25 uF/25 V; "(?)" = not certain; "(computed)" = my arithmetic. The circuit is a Twin Reverb AB763 preamp, mixer, reverb and vibrato with a 2 x 6L6GC output stage and a GZ34 rectifier. Values equal to the Twin AB763 are marked "as Twin".

## 1. Tube complement and heater notes
| Tube | Function |
|---|---|
| 7025 x4 | V1A/V1B Normal stage 1/2, V2A/V2B Vibrato stage 1/2 |
| 7025 x2 halves | Mixer (plate +270 V), reverb recovery (plate +270 V) |
| 12AT7 | Long-tail phase inverter |
| 12AT7 | Reverb driver (triodes paralleled) |
| 12AX7 | Vibrato oscillator and modulator |
| 6L6GC x2 | Output (one per phase, push-pull) |
| GZ34 | Rectifier |
- Heaters: 6.3 V winding with grounded centre tap, "To all 6.3 volt heaters", pilot light. Ground switch with .047-600 cap, AC switch, standby switch.
- Parts list printed: TR1 125P5DX / 125P5D (power), TR2 125C1A (choke), TR3 125A9A (output), TR4 125A20B (reverb transformer).

## 2. Stage by stage
All as the Twin AB763 except where stated.
### Normal channel
- Input jacks 1 and 2: 68k + 68k in series (grid at the junction), 1M from the jack-1 end to ground.
- **V1A**: plate **+270 V**, load **100k**, cathode **1500 || 25 uF/25 V**, **+2.1 V**. Plate to the tone stack (section 3).
- **V1B**: plate **+270 V**, load **100k**, cathode node A **820 || 25-25**, **+2.1 V**. Grid from the Volume wiper.
- Volume **1M-A "2-35"** with **120 pF** bright cap (plus switch/contact as drawn) across top-to-wiper.
- V1B plate -> **.047 uF -> 220k** -> PI input node N1 (as Twin).
### Vibrato channel
- Same input network, V2A/V2B with the same values: plates **+270 V**, V2A cathode 1500 || 25 uF, V2B cathode node A 820 || 25 uF, **+2.1 V**; Volume **1M-A "2-35"** with 120 pF bright cap.
- V2B plate -> **.02 uF** -> node VO; VO -> **3.3M || 10 pF** -> mixer grid; VO -> **500 pF** -> reverb driver grid (as Twin).
### Mixer (1/2 7025)
- Plate **+270 V**, load 100k; grid fed from VO via 3.3M || 10 pF and from the reverb return node (470k / 220k); plate -> **.1 uF** -> N2 -> **220k** -> N1 (as Twin); N1 -> **.001 uF** -> PI top grid (as Twin).
- N2 also feeds the INTENSITY pot (optical vibrato shunt, section 4).

## 3. Tone stack (both channels)
- Topology: Fender TMB. **100k** slope resistor; **250 pF** from the plate to the Treble top; **Treble 250K-A "2-35"**; **.1 uF** (slope node to Treble-bottom/Bass-top); **Bass 250K-A**; **.022 uF** (slope node to Bass-bottom/Middle-top) - **Twin AB763 has .047 uF here**; **Middle 10K-A** (Vibrato channel only).
- **Normal channel: fixed 6800 ohm in place of the Middle pot** (layout shows the resistor on the Bass pot lugs; Normal and Vibrato have separate Bass/Treble pots). Twin AB763 has a full Middle pot on both channels.
- Bright: 120 pF across each Volume (top to wiper) with the bright switch/contact.
- No shift/boost switching.
- Effect (computed, qualitative): .022 uF raises the mid-notch frequency and lowers mid depth relative to the Twin; the fixed 6800 on the Normal channel fixes its mid contour.

## 4. Reverb and vibrato
- **Reverb driver 12AT7** (paralleled triodes): 1M grid leak; 500 pF from VO; cathode **2200 || 25 uF**, **+8.4 V**; plate **+450 V** at the TR4 primary, other end to node D (+410 V). Reverb transformer TR4 125A20B; reverb input/output jacks, pedal jack with 220k (layout shows "220K" near the reverb pedal jack).
- **Reverb recovery (1/2 7025)**: plate **+270 V**, load 100k; plate -> **.003 uF** -> Reverb pot **100K-L**; wiper -> **470k** -> return node; **220k** return node to ground; return node wired to the mixer grid (as Twin AB763; the AA270 added a .002 in series with the 220k grid resistor).
- **Vibrato 12AX7**: oscillator plate **+280 V**, cathode **+2.5 V** (**2700 || 25 uF**); phase-shift chain .02 -> P1 -> .01 -> P2 -> .0? -> P3, 1M from P2 and P3 to node NB, NB -> **2.2M** to the raw negative bias node; SPEED **3.0M RA** (layout) from P1 with 100k to ground; modulator grid from P2; modulator plate **+390 V**, load **10M**, cathode **+17 V**, lamp series **100k** to B (+460 V); INTENSITY **50K-RA** (layout "50K-RA 2-35") from N2 to ground via the photocell as in the Twin. Vibrato pedal jack as Twin.
- The oscillator network cap labelled ".0?" is not resolved (probably .01 as in the Twin) (?).

## 5. Phase inverter (12AT7 long-tail, as Twin AB763 except plate voltages)
- Top grid: **.001 uF** from N1; bottom grid **.1-200** from the NFB node; grid leaks **1M** each to node X; cathodes **+106 V**; tail **470 ohm** (cathodes -> X, X **+104.5 V**), **22k** (X -> NFB node), **100 ohm** (NFB node -> ground).
- Plate loads **82k 5 % / 100k 5 %** to node C (**+450 V**); plates **+230 V / +230 V** (Twin AB763: +245/+235 V).
- Plate outputs **.1 uF** each to the phase nodes.

## 6. Output stage
- **2 x 6L6GC**, cathodes to ground (fixed bias), **1500 ohm** grid stopper per tube, **470 ohm 1 W** screen resistor per tube; plates **+460 V**, screens **+460 V** (printed), OT centre tap **+465 V**.
- Grid returns: **220k 5 %** per phase to the bias line, **-52 V** printed.
- Bias supply: tap on the HV winding via **470-1W** -> diode -> raw node with **25/50** cap (as drawn, "25 / 50"); **10K-L pot + 27K** from the raw node to ground producing the -52 V line; the 2.2M vibrato resistor returns to the raw node.
- OT TR3 125A9A; secondary drawn with **four speakers in parallel** and an **EXT SPKR** jack; individual and total impedance not printed on the pages I read (?).
- **Global NFB: 820 ohm** from the OT secondary hot to the PI NFB node (22k/100 ohm junction), as Twin. Divider 100/(820+100) about 0.109 (-19 dB, computed). No presence control, no NFB cap.

## 7. Supply
- **GZ34** rectifier; TR1 HV secondary printed **360 V AC** each side of centre tap.
- Rail **+465 V** after the rectifier -> **standby switch** -> OT centre tap / plates and screens at **+465 V** (read at the OT end; plates +460 V) -> **TR2 choke** -> node B **+460 V** (20-525) -> **1k 1 W** -> C **+450 V** (20-525; PI plate loads, reverb driver plate) -> **4700 ohm 1 W** -> D **+410 V** (20-525; preamp, reverb, vibrato stages). Same dropper values as the Twin AB763.
- Reservoir: **2 x 70-350** series with **220k 1 W** balance resistors.
- Bias line as section 6. Heater centre tap grounded.
- Sag sources: tube rectifier (GZ34), choke, plates fed before the choke, 2-tube output so lower current than the Twin; B+ falls with load.

## 8. Not legible / uncertain
- Speaker impedances and OT primary impedance (not printed).
- Oscillator network cap ".0?" and the "25/50" bias cap order.
- Bright-switch details; reverb transformer impedances.
- Exact cathode of the second-stage pair on the Normal channel (node A shared) as in Twin.
- Pot taper letters read from the layout (A = audio, L = linear, RA = reverse audio).
