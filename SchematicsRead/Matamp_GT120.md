# Matamp GT120: schematic read stage by stage

Source: `gt120-1.png` (2626 x 1174, one very wide page). Tiled and enlarged into six crops; **nearly every component value is legible**. This is a redrawn schematic (clean vector style, "see tips if using 220V indicator" note, no title block, no model name on the page: the GT120 name comes from the file name, not the drawing). **There are no DC voltages, no signal levels, no transformer winding ratios, no standing currents and no bias voltage printed anywhere on it.** So everything about operating point is computed from stated assumptions or left blank.

Tube numbering on the sheet: V1 ECC83 (two triodes), V2 ECC83 (two triodes), V3 to V6 EL34. Everything marked (computed) is arithmetic from printed values; (assumed) means a tube parameter not on the sheet (ECC83 mu = 100, rp = 62.5 k, gm = 1.6 mS typical). (?) = not legible.

## 1. Signal path overview

Input pair of jacks, V1a gain, a 6-cap voicing selector with a gain pot (1M LOG), V1b gain, a two-pot passive tone stack, an effects loop (SEND / RETURN / SLAVE), V2a gain stage that carries the global NFB, V2b direct-coupled split-load phase inverter, four EL34 in fixed-bias class AB, output transformer with 16 / 8 / 4 ohm taps. **No master volume**: the only level controls are the gain pot (1M LOG), the second 1M LOG pot in the tone stack (see 4) and the 5k LIN presence pot. All controls are unlabelled on the sheet, so "gain", "tone" and "presence" below are my inference from the circuit position.

## 2. Input and V1a

- Two jacks, each through 68 k (R29 on the upper jack, R30 on the lower) into a common grid node; R32 220 k to ground at the grid; R31 1M from the lower jack's tip to ground; the jack switches tie the two together (exact normalling not traceable (?)).
- Input impedance seen per jack (computed): 68 k + 220 k = 288 k. The 68 k with the grid capacitance forms a stopper: with ECC83 Cgk + Miller (about 2 + 1.7 x 79 = 136 pF, assumed Cgp) the pole is 1/(2 pi x 68k x 136 pF) = 17 kHz (computed, assumed capacitances) at the stage's full gain (with the cathode bypassed).
- A 4-position-looking rotary at the grid node (three-pole drawing, mechanically ganged by the dashed line with the other two wafers) selects a path through R33 100 k to the junction of R34 100 k and C27 0.1 uF (to ground). The traced function is partly unclear (?); read as an input attenuator / voicing switch ganged with the cap-selector wafer.
- **V1a**: anode load R2 220 k (supply node via R3 33 k/1 W and C3 10 uF/450 V), cathode R1 2.4 k bypassed by C1 50 uF/25 V (full bypass, corner 1.3 Hz, computed). Gain at full bypass = mu RL / (RL + rp) = 100 x 220 / 282.5 = 78 (37.8 dB, assumed mu, rp). Cathode bias: no cathode voltage printed.
- C2 0.047 uF couples the anode to the **cap selector**.

## 3. Voicing selector and gain pot

- Six capacitors hang off the V1a coupling node, one is switched onto the gain pot: C21 0.1 uF, C22 4700 pF, C23 2700 pF, C24 1500 pF, C25 820 pF, C26 390 pF (each fed from the node after C2, selected by the middle wafer).
- Gain pot: **1M LOG**. High-pass corners of each cap into the pot at full resistance (1 M): 1.6, 34, 59, 106, 194, 408 Hz (computed); with a lower effective load (pot turned down, 100 k): 16, 339, 590, 1061, 1941, 4081 Hz (computed). So the selector is a **bass-cut voicing switch** whose effective cut shifts upward as the gain is turned down.
- Third wafer: C28 680 pF switched between the gain pot's top section and its wiper (a bright cap, treble bleed). The wafer switching contacts are partly untraceable (?).
- R34 100 k (from the selector wiper node to a ground line with C27 0.1 uF): a tail resistor / second bass-cut network of unclear role (?).
- Wiper goes to **V1b grid**, grid leak R35 470 k.

## 4. V1b, tone stack and effects loop

- **V1b**: anode R5 220 k (supply node with C6 16 uF/450 V), cathode R4 2.4 k + C4 50 uF/25 V (bypassed; 1.3 Hz), gain 78 (37.8 dB, assumed). C5 0.047 uF couples the anode to the stack.
- Source impedance of the stack (computed, assumed rp): 220 k || 62.5 k = 48.7 k.
- **Stack (two 1M LOG pots, passive)**, as read and as simulated with nodal analysis (script gt_tone.py):
  - Pot A: top fed from V1b anode through **C7 330 pF**; bottom to ground through **C8 2200 pF**; its wiper is the stack output (to R10 and the loop). Reads as the "treble" control.
  - Pot B: top fed through **R8 100 k** from the V1b anode; its top and wiper bridged by **C10 2200 pF**; wiper-to-bottom bridged by **C9 0.01 uF**; bottom to ground through **R9 27 k**. Reads as the "bass" control.
  - **R7 100 k** from Pot B's wiper to Pot A's wiper. (R8 100 k ties Pot B's top to the anode node.)
  - 1 M log pots: taper assumed 10 % at mid-rotation.
- Computed response (dB re stack input; source 48.7 k; load 10 M || 60 pF assumed for V2a grid); frequencies 30, 60, 120, 250, 500, 1k, 2k, 4k, 8k, 16k Hz:

  | settings (A treble / B bass) | 30 | 60 | 120 | 250 | 500 | 1k | 2k | 4k | 8k | 16k |
  |---|---|---|---|---|---|---|---|---|---|---|
  | 5 / 5 | -19.5 | -18.3 | -16.7 | -16.0 | -16.9 | -18.6 | -19.7 | -20.2 | -20.3 | -20.6 |
  | 0 / 0 | -30.9 | -27.9 | -23.5 | -19.3 | -17.9 | -19.7 | -24.1 | -29.7 | -35.5 | -41.5 |
  | 10 / 10 | -2.0 | -3.4 | -6.1 | -10.4 | -14.8 | -15.0 | -11.1 | -7.7 | -6.2 | -5.7 |
  | 10 / 0 | -31.2 | -28.5 | -24.2 | -19.1 | -15.1 | -12.4 | -9.8 | -7.4 | -6.1 | -5.7 |
  | 0 / 10 | -1.8 | -3.0 | -5.9 | -10.8 | -15.4 | -19.3 | -24.1 | -29.6 | -35.5 | -41.5 |
  | 10 / 5 | -19.7 | -18.8 | -16.8 | -14.9 | -13.9 | -12.6 | -10.1 | -7.5 | -6.1 | -5.7 |
  | 5 / 10 | -1.8 | -3.1 | -6.1 | -11.4 | -16.3 | -18.9 | -19.9 | -20.2 | -20.4 | -20.6 |

  Swings (computed): bass pot 24.9 dB at 60 Hz and 17.6 dB at 120 Hz, essentially nothing above 1 kHz; treble pot 22 dB at 4 kHz and 29 dB at 8 kHz, 6.5 dB at 1 kHz, nothing below 500 Hz. Mid scoop is fixed: the response bottoms around 500 Hz to 1 kHz (-15 to -19 dB with everything up), and there is **no mid control**: this is a bass and treble stack only (Orange-like Tone: bass and treble, big mid scoop in the passive network). With both at 10 the insertion loss at 1 kHz is -15 dB; at 5 / 5 it is -18.6 dB (computed).
- Output W goes through **R10 100 k** to the V2a grid. **R11 220 k** to the SEND jack, **R12 100 k** from the RETURN jack onto the same grid node, and a SLAVE jack on the grid side of R10 (jack switch details not fully traceable (?)). So the loop is **post-tone-stack, pre-V2a**, with a 220 k source impedance for the send (a very light tap, unbuffered).

## 5. V2a: voltage stage with the global NFB

- **V2a**: anode load R13 **390 k** (supply node with C12 16 uF/450 V and R6 10 k/1 W before it). Cathode **R14 1.5 k** to ground; in parallel the **presence pot, 5k LIN**, whose wiper goes to ground through **C29 0.1 uF**; a second wiper branch **C16 0.1 uF in series with L1** (inductor, value not printed (?)) to ground. The cathode is **not bypassed** (feedback point).
- **NFB**: R15 **24 k in parallel with C13 330 pF** from the **16 ohm** output-transformer terminal to the V2a cathode node (feedback is from the full 16 ohm winding regardless of the speaker tap in use).
  - Feedback fraction at low frequency: R14 / (R14 + 24 k) = 5.9 % (-24.6 dB) if the presence pot is ignored; with the 5k pot (to ground along its element, wiper at ground end) the cathode resistance is 1.5 k || 5 k = 1.15 k and beta = 4.6 % (-26.8 dB) (computed). The 330 pF adds a zero at 1/(2 pi x 24k x 330p) = 20 kHz.
  - Presence action (computed from the drawn network, C29 only; the L1/C16 branch is a series-resonant trap whose frequency cannot be computed without L1): with the wiper at the **cathode end**, C29 sits across the 1.5 k and beta falls from 4.58 % at 100 Hz to 3.77 % at 1 kHz and **0.73 % at 10 kHz** (feedback reduced by 16 dB at 10 kHz, so the open-loop presence peak is revealed). With the wiper at the **ground end** it does nothing (beta 4.59 % to 5.12 %). Corner of the C29 effect with the pot at the cathode end alone: 1/(2 pi x 1.5k x 0.1u) = 1.06 kHz.
- Gain of V2a without feedback (computed, assumed mu, rp; cathode DC resistance 1.15 k): 100 x 390 / (390 + 62.5 + 101 x 1.15) = 68 (36.7 dB).
- **C11 680 pF from the V2a anode to the supply (AC ground)**: reads as a plate-load shunt (I could not rule out a different intent): with 390 k || 62.5 k (assumed rp) = 53.9 k the pole is at 1/(2 pi x 53.9k x 680p) = 4.3 to 4.5 kHz (computed, assumed rp): a fixed first-order treble cut of about -3 dB at 4.4 kHz and -7.7 dB at 10 kHz, **inside the global NFB loop**. (Medium confidence in the connection.)
- **V2a anode drives the V2b grid directly** (DC-coupled).

## 6. V2b: split-load phase inverter

- Plate load **R16 100 k** (supply node as V2a), cathode load **R17 100 k** to ground: equal loads, gain per side = mu R / (rp + R + (mu + 1) R) = 100 x 100 / (62.5 + 100 + 10100) = 0.97 (computed, assumed). It is the usual split-load "concertina". Because the grid is direct-coupled to V2a's anode, the cathode follows at roughly V2a plate voltage minus grid bias: DC levels not printed (?).
- Outputs: **C14 0.047 uF** from the plate to the upper EL34 pair; **C15 0.047 uF** from the cathode to the lower pair.
- Power-tube grid leaks **R18 220 k and R19 220 k** (series between the two coupling nodes; midpoint goes to the bias supply). The coupling corner is 1/(2 pi x 220k x 0.047u) = 15.4 Hz (computed).
- Output grids through **2.2 k stoppers (R36 to R39)** each.

## 7. Power stage: four EL34, fixed bias

- V3, V4 one phase (parallel, upper side); V5, V6 the other. Plates to the output transformer primary ends, primary centre tap = node **A+**. **Screens through 1 k / 5 W each (R40 to R43) to B+**, which is *after* the choke and fuse (A+ is the main reservoir node). Cathodes direct to ground (fixed bias).
- **Bias supply**: a separate winding (the secondary at the top left of the transformer sketch; voltage not printed (?)) into **D9 1N4007**, **R24 27 k**, **C18 10 uF/150 V**, **R23 15 k**, **C17 10 uF/150 V**; **R21 33 k** to ground; **R22 100 k** and **R25 100 k pot** in parallel between that node and the negative node; the **pot wiper goes to the R18/R19 midpoint**. Resulting adjustment (computed): the wiper can move between **40 % and 100 %** of the negative node's voltage (33 k / (33 k + 50 k) = 0.40, where 50 k is R22 || R25). The absolute bias voltage is **not computable** (winding voltage not printed). The two 150 V capacitors bound |V_bias| below 150 V (computed from the ratings).
- Bias is negative-only from the rectified bias winding: hence **no cathode resistor, no bypass, no per-tube trim**: all four tubes share one setting.
- **Output transformer**: taps **16 ohm, 8 ohm, 4 ohm**, selected by a rotary to two parallel jacks. The 16 ohm terminal is the NFB take-off. Primary impedance, ratio and DC resistance are **not printed** (?).

## 8. Power supply

- Mains selector **110 / 118 / 220 / 240 V** primary taps; fuse "5A at 118VAC and 110V, 3A otherwise"; switch and an IEC-style L/N/E block.
- **Bridge D1 to D8: "1N4007 x8"** as drawn (one bridge symbol with the note x8: an eight-diode series/parallel build) into reservoir **C30 and C31, 100 uF / 500 V each, in series** (50 uF total, computed), with the winding centre tap tied to their midpoint; the output node is **A+**. Secondary voltages are not printed (?).
- **Choke** (value not printed (?)) in series with a **1 A fuse** from A+ to **B+**; **C19 and C20 33 uF / 500 V in series** at B+ (16.5 uF total, computed) with **R26 and R27 100 k** balancing resistors.
- Preamp supply chain from B+: **R20 10 k/1 W** to the V2 node (C12 16 uF/450 V; time constant 0.16 s, computed), then **R6 10 k/1 W** to the V1b node (C6 16 uF/450 V; 0.16 s), then **R3 33 k/1 W** to the V1a node (C3 10 uF/450 V; 0.33 s): three cascaded RC decouplers.
- Heaters: **6.3 V for the two ECC83** (with a hum-balance pot R28 100 ohm and a pilot indicator) and a separate **6.3 V for the four EL34** with a grounded centre tap.
- Sag: **cannot be computed**. No transformer secondary voltage, DC resistance or rectifier voltage is printed. What is on the page: a **solid-state rectifier** (no tube rectifier sag), two stiff reservoirs (50 uF at A+ and 16.5 uF at B+ behind a choke), and screens drawn from the post-choke node while plates take the pre-choke node. So there is **choke-input-like filtering for the screens and preamp** and a plain capacitor-input reservoir for the plates, which gives modest plate-supply sag (silicon rectifier, no series impedance on the plates other than the transformer) and more screen-supply ripple/sag than plate sag.

## 9. Gain chain (computed, all tube parameters assumed)

| stage | gain | note |
|---|---|---|
| input divider (68 k + 220 k) | 0.76 (-2.4 dB) | |
| V1a | 78 (37.8 dB) | bypassed |
| gain pot (1M LOG) | 0 to 1 | wiper into V1b |
| V1b | 78 (37.8 dB) | bypassed |
| tone stack | -16 dB (5 / 5, 250 Hz) to -18.6 dB (1 kHz) | up to -2 dB (10 / 10 bass) |
| V2a | 68 (36.7 dB) open loop at the cathode, no NFB | then inside the loop |
| V2b | 0.97 per side | |

Product at gain full, stack 5 / 5, 1 kHz: 0.76 x 78 x 78 x 0.117 x 68 x 0.97 = 3.6 x 10^4 (about 91 dB) before NFB (computed, assumed). That is roughly 90 dB of drive ahead of a push-pull stage that clips at a few tens of volts, so with the gain up the preamp tubes and the power stage are overdriven by anything above a few mV at the input: **a very hot cascaded-triode preamp with a passive bass/treble stack placed after two gain stages.** Do not trust the figure beyond the tube assumptions.

## 10. What could not be read or is not given

- All DC voltages, operating currents, the bias voltage, transformer secondary and primary data, choke value, L1 value.
- The exact switching in the three-wafer selector (which contact feeds R33 100 k, R34 100 k, C27 and C28), and the jack-switch normalling of SEND / RETURN / SLAVE.
- The model name and any title block.
- The value and polarity of the lamp / indicator, "see tips" note.
