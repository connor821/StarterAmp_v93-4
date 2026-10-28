# "Ross Vintage Compressor" schematic: circuit read

Source: `misc/ggg_dnr_ross_sc-1.png` (1651 x 1275, one page). Title block: "Ross(TM) Vintage Compressor Schematic", version 2011 November 08, copyright 2009 JD Sleep, General Guitar Gadgets (generalguitargadgets.com). The sheet states that posting or serving the file from any other site is refused, so these notes only record values and my reading; the drawing itself is not reproduced anywhere. The text extraction (`ggg_dnr_ross_sc.txt`) holds only the title block, so everything below is from the image, read at 2x. "(?)" = not certain.

## 1. Identification

- **OTA-based, CA3080.** IC1 is a **CA3080** operational transconductance amplifier (pins 2 and 3 = inputs, 5 = amplifier bias current ABC, 6 = output, 7 = V+, 4 = V-). Five NPN transistors **Q1 to Q5 = 2N5089**, diodes **D1 to D3 = 1N914** (D1 is across the supply as a polarity guard).
- It is the **MXR Dyna Comp family** (CA3080 in front of a transistor full-wave detector with a 10 uF reservoir and a 150k recharge), with a different input stage and an extra buffer. This identification is mine (from the topology and the values below), not stated on the sheet. The repo's Dyna model assumes 10 uF and 150k recharge (about 1.5 s) and a 1M / 10 nF smoothing (about 10 ms), and both of those numbers appear on this schematic (R18 150k, C16 10 uF; R17 1M, C14 0.01 uF), see `Compressors_vs_code.md`.
- Controls: **VR1 Sustain 500k linear**; **VR2 Level 50k log**. No attack or release control (fixed by the RC values). **Tr1 2k trimmer** (balance / bias trimmer at the OTA input).
- Power: 9 V battery B1 (or the DC 9 V jack J3), filtering **C9 10 uF**, **C12 0.01 uF**, polarity diode **D1 1N914**. Switch **Sw1a / Sw1b / Sw1c** (3PDT true bypass: input, output and LED), LED indicator with **R22 1k**. Jacks J1 (input), J2 (output).

## 2. Signal path

### 2.1 Input
- J1 -> Sw1a -> input node with **C2 220 pF** to ground (RF), **R1 1M5** to ground, and **C1 0.01 uF** in series with **R2 10k** into Q1 base.
- **Q1 (2N5089)**: emitter follower. Base bias: **R3 470k** from the base to a node that is held by **R4 470k** to the +9 V rail and **C4 1 uF** to ground (so the base sits at about 4.5 V, the 470k/470k divider). Emitter: **R5 10k** to ground. The collector is fed from a filtered supply (**R9 10k** with **C5 1 uF** to ground).
- High-pass at the input: C1 0.01 uF into about 10k plus the bias network: roughly 1.6 kHz to about 10 Hz depending on which impedance dominates; the input impedance of the divider and R1 is about 200 kohm (R1 1M5 in parallel with the 235k base bias, derived).

### 2.2 OTA input network and the trimmer
- Q1 emitter -> **C6 1 uF** (electrolytic, + toward the OTA) -> node A. Node A: **R8 1M** to ground, and IC1 **pin 2 (inverting)** directly.
- **Tr1 2k trimmer** from node A to node B; node B = IC1 **pin 3 (non-inverting)**, **R7 1M** to ground.
- Node B also has the shunt network to ground: **C10 0.01 uF in parallel with R11 15k**, then **C8 1 uF** to ground (so a 15k shunt that opens at DC and at very low frequencies and is joined by 0.01 uF above about 1 kHz).
- The wiper of Tr1 gets a bias through **R6 220k** from a node formed by **R10 220k** (from the +9 V side of R9) and **C7 1 uF** to ground: a heavily filtered DC feed that nulls the OTA offset (the trimmer balances the two inputs). Q1's collector supply is the other side of **R9 10k**, with **C5 1 uF** to ground.
- Because the signal is only a small difference across the 2k trimmer (the rest is attenuated by the 1M resistors), the OTA input differential voltage is small, which keeps the CA3080 in its linear range (about 20 mV). Derived: with 2k against 1M the attenuation is about 54 dB, the shunt branch (about 15k when active) raises the differential drive in the band where the shunt is active (a pre-emphasis effect for the compressor), (?) by how much.

### 2.3 OTA output and the following buffer
- IC1 **pin 6** (output current) -> node with **R14 150k** to the filtered supply node (**R13 56k + R15 27k + C3 1 uF** make the supply for it: R13 56k from +9 V, R15 27k onward, **C3 1 uF** to ground) and **C11 0.001 uF** from the output node to the same filtered node: the OTA's load is R14 150k with 1 nF across it, corner 1/(2 pi 150k 1n) = 1.06 kHz (derived), so the OTA output is a lowpass with a pole near 1 kHz before the next stage.
- **Q5 (2N5089)** base is that output node: an emitter-follower/phase-splitter stage. **R19 10k** collector load (to the +9 V line), **R21 10k** emitter to ground. The collector gives an inverted copy, the emitter a non-inverted copy, so Q5 produces both polarities for the full-wave detector (gain about 1 each, derived). 
- Audio out: Q5 emitter -> **C15 0.047 uF** -> **R20 10k** in series -> **VR2 Level 50k log** (wiper to Sw1b, output jack J2). C15 with R20 plus the pot: 0.047 uF into 10k to 60k: about 56 Hz to 340 Hz corner.

## 3. Detector and gain control (the attack and release network)

- **Q2 and Q3 (2N5089)** pull down the control node; both have their emitters at ground and their collectors tied to the same node (the node that carries C16 and the base of Q4).
- **Q3 (non-inverted half):** Q5 emitter -> **C14 0.01 uF** -> Q3 base. Base node: **D3 (1N914)** to ground and **R17 1M** to ground. The diode clamps the negative side so the base gets only positive peaks; the 1M discharges the coupling cap: **R17 x C14 = 1M x 0.01 uF = 10 ms** (this is the attack-side smoothing time constant).
- **Q2 (inverted half):** Q5 collector -> **C13 0.01 uF** -> **D2 / R16** node: **D2 (1N914)** to ground, **R16 1M** to ground, then to Q2 base. (The schematic prints "R16 1M" and "C13 0.01 uF" at that node.) Same arrangement, same 10 ms product. (The reading of which of the two 10k/1M resistors belongs to which side of the Q2 branch is clear in the figure: R19 10k is the collector load of Q5 and R16 1M is the bias leg.)
- **Reservoir:** **C16 10 uF** (electrolytic) from the control node to ground. **R18 150k** from +9 V to the control node (through the base of Q4 at its bottom end): the recharge path. **Recharge time constant: R18 x C16 = 150k x 10 uF = 1.5 s** (derived). So the release (gain recovery) is about 1.5 s with no knob.
- **Attack:** the transistors discharge C16 as fast as the peaks demand (a large overload pulls it down faster), after the 10 ms smoothing by C14/R17 and C13/R16. So attack is signal-dependent and fast; there is no attack control.
- **Q4 (2N5089)** is an emitter follower from the control node (base at the R18 / C16 node, collector at +9 V), feeding the **Sustain pot VR1 500k linear**. The pot is wired as a variable series resistor: Q4 emitter -> VR1 (0 to 500k) -> **R12 27k** -> IC1 **pin 5** (ABC). The control current is then roughly (V_node - 0.65 - 0.65) / (R12 + VR1): about 0.3 mA at VR1 = 0 down to about 16 uA at 500k (derived, assuming a 9 V node). More series resistance means less ABC current, so less OTA transconductance and lower gain in the circuit overall. So Sustain acts on the gain control rather than on a threshold, and it behaves as the sensitivity (more sustain = a lower resistance = more gain control swing, which makes the compression start earlier and go deeper).
- Because the detector listens to **Q5, which follows the OTA output**, this is a **feedback** (closed-loop) detector like the CS-3. Dyna Comp family circuits are commonly described this way.

## 4. All values (as read)

| Ref | Value | Role |
|---|---|---|
| C1 | 0.01 uF | input coupling |
| C2 | 220 pF | RF shunt at input |
| C3 | 1 uF | filter on R13/R15 supply |
| C4 | 1 uF | Q1 bias filter |
| C5 | 1 uF | Q1 collector filter (with R9) |
| C6 | 1 uF | Q1 emitter to OTA |
| C7 | 1 uF | trimmer-bias filter (with R10) |
| C8 | 1 uF | OTA pin 3 shunt network |
| C9 | 10 uF | supply |
| C10 | 0.01 uF | OTA pin 3 shunt (parallel with R11) |
| C11 | 0.001 uF | OTA output load |
| C12 | 0.01 uF | supply bypass |
| C13 | 0.01 uF | Q2 coupling |
| C14 | 0.01 uF | Q3 coupling |
| C15 | 0.047 uF | audio out coupling |
| C16 | 10 uF | envelope reservoir |
| R1 | 1M5 | input to ground |
| R2 | 10k | series into Q1 base |
| R3 | 470k | Q1 base bias |
| R4 | 470k | Q1 bias divider top |
| R5 | 10k | Q1 emitter |
| R6 | 220k | trimmer bias |
| R7 | 1M | OTA pin 3 to ground |
| R8 | 1M | OTA pin 2 side to ground |
| R9 | 10k | Q1 collector supply |
| R10 | 220k | trimmer-bias divider |
| R11 | 15k | OTA pin 3 shunt |
| R12 | 27k | pin 5 (ABC) series |
| R13 | 56k | OTA output load supply |
| R14 | 150k | OTA output load |
| R15 | 27k | OTA output load supply |
| R16 | 1M | Q2 base bias |
| R17 | 1M | Q3 base bias |
| R18 | 150k | envelope recharge |
| R19 | 10k | Q5 collector load |
| R20 | 10k | series ahead of Level |
| R21 | 10k | Q5 emitter |
| R22 | 1k | LED series |
| VR1 | 500k linear | Sustain |
| VR2 | 50k log | Level |
| Tr1 | 2k | OTA balance trimmer |
| IC1 | CA3080 | OTA |
| Q1 to Q5 | 2N5089 | NPN |
| D1 to D3 | 1N914 | |

Not readable or not certain: none of the values; the connections flagged with (?) in the text (the exact supply arrangement of R13/R15/C3 relative to the 9 V rail, whether R14's other end goes to the filtered node or directly to the rail, and the Q2-branch node naming).

## 5. Modelling notes (derived, not on the sheet)

- Fixed envelope: smoothing 10 ms (C14 x R17, C13 x R16), recovery about 1.5 s (R18 x C16). Sustain changes the control current only, so the gain law is gain proportional to the control current.
- The high-pass at the detector is the coupling at 0.01 uF into 1M/the base: low frequencies are effectively passed once the base node is clamped, so the 10 ms is the time constant that matters.
- The lowpass at the OTA output (1 kHz) and the shunt network at OTA pin 3 are the circuit's built-in tone shaping, so the Dyna's "high-frequency lift" seen elsewhere in the repo notes is consistent with the C10/R11 branch (a +/- few dB tilt with a corner near 1 kHz), (?) the exact curve was not computed.
