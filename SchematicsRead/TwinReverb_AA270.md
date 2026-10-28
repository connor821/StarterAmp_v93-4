# Fender Twin Reverb-Amp AA270 (silverface) - schematic read

Source: `817f5313-Fender_Twin_Reverb_Amp_AA270.pdf` (1 page scan, 1729x1446). Header: "Fender Musical Instruments, CBS Musical Instruments, a division of Columbia Broadcasting Systems Inc., Fullerton, California". Title block: "Schematic & Wiring Diagram - Twin Reverb, Tube Type", dwg no. 045377, size D, **Rev E**, sheet 1 of 2, "Fender Musical Instruments, Santa Ana, California". Drawn "F.J." (date reads 7-30-6?(?)). Revision list: C "Incorporated ECO 1063", D "Incorporated ECO 1687 (?)", E "Inc. ECO 2250 & ECO 2053 (?), 1-30-70 (?)". Notes: 1) voltages read to ground with VTVM; 2) resistors 1/2 W 10 % unless specified; 3) caps at least 400 V; 4) **voltages measured with the vibrato pedal grounded to chassis**. Mains printed "120 V 50/60 Hz".
Conventions as in the AB763 file ("1500" = 1.5k etc.; "(?)" uncertain; "(computed)" is my arithmetic). Tube reference designators are printed on this sheet.

## 1. Tube complement and heater notes
| Ref | Tube | Function |
|---|---|---|
| V1A / V1B | 7025 | Normal channel stage 1 / stage 2 |
| V2A / V2B | 7025 | Vibrato channel stage 1 / stage 2 |
| V4A | 1/2 7025 | Mixer (vibrato channel + reverb return -> PI feed) |
| V4B | 1/2 7025 | Reverb recovery |
| V3 | 12AT7 | Reverb driver (triodes paralleled) |
| V5 | 12AX7 | Vibrato oscillator + modulator |
| V6 | 12AT7 | Phase inverter (long-tail) |
| V7, V8, V9, V10 | 6L6GC | Output, 2 per phase |
| (rectifier) | solid state, 6 diodes | no tube rectifier |
- Heater winding 6.3 V: **two 100 ohm resistors** drawn from the two heater leads to ground (artificial centre tap); the transformer's middle heater terminal is not shown connected (?). (AB763 instead showed a grounded centre tap, "INTERNAL GROUND".)
- Mains: AC receptacle with neon, ground switch with .047-600 uF cap ("U.L." printed), AC switch, 2 1/2 A slo-blo fuse.

## 2. Stage by stage
Voltages differ from the AB763 because B+ is about 55 V lower.
### Normal channel
- Input: jacks 1 and 2, 68k, 68k in series (V1A grid at the junction), 1M from jack-1 end to ground (same as AB763; jack-switch routing drawn as a curved contact, not resolved).
- **V1A (7025)**: plate **+210 V**, plate load **100k 1 W** from node D; cathode **1500 || 25 uF**, **+1.6 V**. Plate goes directly to the tone stack.
- Tone stack, volume and bright: see section 3. Volume 1M-A "2-35".
- **V1B (7025)**: grid at volume wiper; plate **+210 V**; plate load **100k** (1/2 W) from D; cathode node "A" **+1.6 V** (shared with V2B, 820 || 25 uF).
- Output: **.047 uF -> 220k** -> PI input node N1 (same as AB763).
### Vibrato channel
- Input as Normal. **V2A**: plate **+210 V**, load **100k 1 W**, cathode 1500 || 25 uF, +1.6 V. **V2B**: plate **+210 V**, load 100k, cathode node A **820 || 25 uF**, +1.6 V.
- V2B plate -> **.02 uF** -> node VO; VO -> **3.3M || 10 pF** -> grid of V4A; VO -> **500 pF** -> V3 grid (reverb send, vibrato channel only).
### V4A mixer (1/2 7025)
- Plate **+210 V**, load **100k** from D; cathode **+1.6 V** on node "E" (shared with V4B, 820 || 25 uF).
- Grid: 3.3M || 10 pF from VO, plus reverb return node (470k/220k) wired to the grid (as AB763).
- Plate -> **.1 uF** -> N2 -> **220k** -> N1; N1 -> **.01 uF** -> PI top grid (AB763: .001 uF). N2 also feeds the INTENSITY pot (optical vibrato).

## 3. Tone stack (both channels)
Identical values to AB763: 100k slope resistor; 250 pF bright-ish cap from plate to Treble top; Treble 250k-A "2-35"; .1 uF (slope node to Treble-bottom/Bass-top); Bass 250k-A; .047 uF (slope node to Bass-bottom/Middle-top; label reads ".047", small ".04T" print); Middle 10k-A; Volume 1M-A "2-35" with 120 pF bright cap and switch. No shift/boost switching.

## 4. Reverb and vibrato
- **V3 12AT7** (two triodes in parallel): grid 1M to ground; 500 pF input from VO; cathode **2200 || 25 uF**, **+10 V**; plate "+400 V" at the TR4 primary, other end of TR4 primary to node B. TR4 secondary to reverb pan input.
- **V4B** (reverb recovery): grid from pan output; to the grid node: **220k in series with .002 uF to ground** (printed ".002" over "220K"; AB763 had only 220k to ground) (?: series RC to ground as drawn); plate **+210 V**, load 100k from D; cathode 820 || 25 uF, +1.6 V (node E); plate -> **.003 uF** -> **Reverb pot 100k-L**; wiper -> **470k** -> return node R; **220k** R to ground; R wired to V4A grid. Reverb pedal shorts the grid to ground.
- **Vibrato (V5 12AX7)**: same RC network as AB763: oscillator plate -> .02 -> P1 -> .01 -> P2 -> cap ".0?" (probably .01) -> P3 = grid; P2 -> 1M -> NB, P3 -> 1M -> NB; NB -> **2.2M** to the raw negative bias node (-52 V); SPEED 3M-RA from P1 through 100k to ground; modulator grid from P2. Oscillator: plate **+245 V** (AB763 +300 V), load 220k, cathode 2700 || 25 uF, **+2 V**. Modulator: plate **+385 V**, cathode **+18 V (?)** 100k || 25 uF; plate load **10M**, with a **new cap, about .012 uF (?)** in parallel with the 10M; lamp in series with 100k to B; photocell to ground and to INTENSITY 50k-RA wiper (pot top to N2, bottom ground). VIBRATO PEDAL jack drawn as on AB763.

## 5. Phase inverter: V6 12AT7 long-tail (changed from AB763)
- Top grid: **.01 uF** from N1 (AB763 .001); grid leaks **330k** each (AB763 1M) to node X; bottom grid **.1 uF / 200 V** from the NFB node.
- Tail: cathodes (**+120 V**) -> **270 ohm** (AB763 470) -> X -> **22k** -> NFB node -> **100 ohm** -> ground (total about 22.4k, computed; about 5.4 mA total, computed).
- Plate loads **47k and 47k** (AB763 82k/100k) from node C; plates **+245 V** and **+245 V**.
- Plate outputs through **.1 uF / 600 V** each (AB763 .1) to the phase nodes.

## 6. Output stage
- 4 x 6L6GC (V7, V8 one phase; V9, V10 other phase); cathodes to ground; plates **+405 V** (blue wire label), screens via **470 ohm 1 W** each from B (**+405 V** printed); **1500 ohm** grid stopper per tube.
- Phase node to ground: **2000 pF per phase** (new; AB763 none). Grid return: **68k per phase** (AB763 220k) to the bias network below.
- Bias: **fixed**. Raw negative node **-52 V** (printed); wiper node **-44 V** (printed). The output grid returns are the two ends of a **10k-L pot** ("BIAS" pot; wiper to the -44 V node); the right-hand end also has **15k to ground** (?). Topology as read; interpretation (computed): bottom-phase grid return = wiper node voltage (-44 V); top-phase return = divider of wiper node through the pot segment and 15k (between -44 V and about -26 V). I do not trust this interpretation; the actual grid-bus voltages are not printed.
- Bias supply: tap on HV winding (shown with a **1k (?)** resistor, AB763 470 ohm) -> diode -> -52 V node with **50/70 (uF/V order not resolved, ?)** cap to ground; **3.3k** from -52 V node to the wiper node; **50/70** cap on the -44 V node to ground.
- OT TR3 (export table: TR3-125A29A; same parts list as AB763: TR1-125P34AX / 125P34A, TR2-125C1A, TR4-125A20B). Printed "**TOTAL LOAD 4 ohm**"; two speakers drawn plus EXT SPKR jack; individual speaker impedance not printed. OT primary impedance not printed.
- **Global NFB**: **820 ohm** from OT secondary hot to the PI NFB node (22k/100 ohm junction) (same as AB763; no NFB cap). No presence control.

## 7. Supply
- TR1 HV secondary printed **315 V AC** per side (AB763 340 V AC); 6 diodes (3 per leg).
- Reservoir **2 x 70 uF / 350 V** in series, each with **220k 1 W** balance resistor (same as AB763).
- Rail before standby: printed "**+410 V - STANDBY ON**" and "**+450 V - STANDBY OFF**" on the same wire (meaning of ON/OFF not resolved).
- After standby: plates/OT centre tap at **+405 V**; choke **TR2** to node **B (+405 V)**, 20 uF / 525 V.
- B -> **2.2k 1 W** -> **C (+380 V (?))**, 20 uF / 525 V (feeds PI plate loads). AB763: 1k.
- C -> **10k 1 W** -> **D (+315 V (?))**, 20 uF / 525 V (preamp). AB763: 4700.
- Consistency check (computed): PI about 5.4 mA + preamp about 6.5 mA through 2.2k gives about 25 V drop, matching 405 -> 380.
- Sag sources: solid-state rectifier, TR2 choke, 2.2k and 10k RC chain (note the larger 10k is not on the plate path), 6L6 plates ahead of the chain.

## 8. Not legible / uncertain
- D (+315 V) and C (+380 V) printed values (read, supported by computed check).
- Bias network interpretation (see section 6); the 1k tap resistor (could be another value); the 50/70 capacitor marking order.
- Modulator cathode (+18 V or +16 V), the parallel cap across 10M (".012"), the P2-P3 cap (".0?").
- Rev block dates/ECO numbers (D, E lines).
- "Standby ON/OFF" voltages meaning.
- Speaker impedances individually; OT primary impedance; sheet 2 of 2 (wiring/layout) is not in this file.
