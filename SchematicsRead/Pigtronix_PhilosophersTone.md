# Pigtronix Philosopher's Tone Micro: schematic read, with derived detector and compression numbers

Source: `pigtronix-1.png` (2420 x 1870, fully legible). **Important provenance:** the page is not a Pigtronix factory drawing. Its title block reads "aion / Pigtronix Philosoper's Tone Micro / Schematic v1.0 (3/28/2025) / aionfx.com": a **third-party trace** of the pedal (spelling "Philosoper's" as printed). Values are the tracer's. One part is marked **"unknown"** (Z1, see 8). Everything marked (computed) is arithmetic from printed values. (assumed) marks device data that is **not** on the sheet: the VTL5C6 LDR curve, 1N914 and LED I-V curves, op-amp output resistance. I could not find a VTL5C6 datasheet (searches returned other VTL5C parts), so the LDR is modelled two ways and the results are given as ranges.

## 1. Block diagram (all legible)

```
IN -- R25 10M to GND
   -- C1 22n -- R2 10k -- IC1A(+)  [R1 470k to VB bias]       IC1A NE5532 unity buffer ("dry")
IC1A out -- BLEND pin 1 (dry)
IC1A out -- C2 1uF -- SUSTAIN 100kB (rheostat) -- R5 2k7 -- IC1B(-)      IC1B NE5532 inverting, (+) at VB
IC1B feedback: R7 2M2 || C3 33pF || VTL5C6 LDR                           (LED driven from IC1B out)
IC1B out -- R3 220k -- node N -- IC2A(+)   IC2A TL074 unity buffer
   N to GND via: R6 100k + R8 27k + C5 1uF (small signal),
                 and 3x1N914 antiparallel stacks (D1,D3,D5 / D2,D4,D6) across R6
IC2A out -- R14 2k7 -- [C9 2n2 to GND] -- R15 100k -- IC2B(-)           IC2B TL074 inverting, R19 68k feedback, (+) at VB
IC2B out -- BLEND pin 3 (wet)
BLEND wiper -- R12 8k2 -- [C7 680pF to GND] -- IC2C input (treble stage)
IC2C TL074 non-inverting, R18 10k feedback, TREBLE 100kB with C8 1n5 + R20 2k7 to VB
IC2C out -- C6 2.2uF -- VOLUME 10kA -- R17 150R -- OUT
```
IC2D: unused (follower with + input grounded). The dry and wet signals are both non-inverted at the blend (IC1B and IC2B each invert, IC1A and IC2A do not).

## 2. Stage by stage

**Input**: R25 10 M to ground, C1 22 nF into R2 10 k then the IC1A + input with R1 470 k to VB (bias). Input impedance about 480 k (R1 + R2, computed). High-pass 1/(2 pi x 470 k x 22 nF) = **15.4 Hz** (computed). IC1A is a NE5532 follower. No input clipping parts, no switchable boost.

**Sustain gain stage (IC1B)**: inverting, non-inverting input at **VB**. Input path: IC1A out, C2 1 uF, SUSTAIN pot **100 kB (linear taper; used as a variable series resistor: wiper tied to pin 1)**, R5 2.7 k. So Rin = 2.7 k + (0 to 100 k). Feedback: **R7 2.2 M in parallel with C3 33 pF in parallel with the VTL5C6 LDR** (the LDR is drawn in the feedback loop; the dark LDR value is not printed). Gain = -Rfb / Rin:

| SUSTAIN pot position (of 100k) | Rin | max gain (Rfb 2.2 M, LDR dark) | in dB | C2 high-pass corner |
|---|---|---|---|---|
| 0 % (max sustain) | 2.7 k | 815 | 58.2 | 59 Hz |
| 5 % | 7.7 k | 286 | 49.1 | 21 Hz |
| 10 % | 12.7 k | 173 | 44.8 | 12.5 Hz |
| 25 % | 27.7 k | 79 | 38.0 | 5.7 Hz |
| 50 % | 52.7 k | 42 | 32.4 | 3.0 Hz |
| 75 % | 77.7 k | 28 | 29.0 | 2.0 Hz |
| 100 % (min sustain) | 102.7 k | 21.4 | 26.6 | 1.55 Hz |

All computed. Because the pot is linear in series with a small fixed resistor, the sustain control is very non-linear in gain: the first 10 % of rotation covers 13 dB of gain.

C3 33 pF sets a pole with the feedback resistance: **1/(2 pi Rfb C3)** = 2.19 kHz (Rfb 2.2 M, dark LDR), 4.8 kHz (1 M), 7.0 kHz (0.69 M), 48 kHz (100 k) (computed). So **at low level (LDR dark, gain high) the stage has a first-order low-pass near 2 kHz that opens up as the LDR conducts**; the top end is darkest on quiet notes and the end of decays, brightest while the compressor is working hard. At minimum-sustain gain (21.4) the dark-state response is 13 dB down at 10 kHz (2.2 M / 4.68 = 470 k against 2.2 M, computed).
Output swing limited by the rails: IC1B runs between 0 and **VA about 17.4 V** with VB about 9.1 V, so about +/-7.4 V peak usable (5.2 V rms, +14.4 dBV) before the NE5532 hits its rails (computed, op-amp headroom assumed 1 V).

**Level detector and LED drive**: from IC1B out, **D8 and D10** from the output side and **D7, D9** from the reference side form a **full-wave bridge** around the LED (VTL5C6 LED), referenced to **VC** (the 9.1 V LM741 buffered reference) through **R9 47 ohm**. **C4 2200 uF** sits across the LED (the bridge DC side). So the LED sees the rectified (IC1B out minus VC) with a **2200 uF smoothing capacitor in parallel**: a peak-ish detector with a very long hold. Diodes 1N914 (assumed forward curve Is 2.7 nA, n 1.84).
- Detector knee (computed with assumed LED I-V: V_led = 1.65 V + 0.05 V x ln(I / 20 mA); two bridge diode drops; R9 + source resistance about 57 ohm): the bridge starts to deliver meaningful current when the IC1B output reaches **about 2.9 V peak (2.0 V rms, +6.2 dBV)**. Below that the LED is dark.
- Charge time constant: 57 ohm (R9 + ~10 ohm source) x 2200 uF = **0.125 s** (computed; the summary figure of R9 x C4 = 0.103 s uses 47 ohm only).

**Dynamics (computed, time-domain model of C4 + LED + LDR; LDR time response assumed 3 ms rise / 50 ms fall; model A LDR R = 55 k x (I / 1 mA)^-0.90 and model B 6.7 k x (I / 1 mA)^-0.61, both clamped between 0.5 k and the dark value; dark Rfb 2.2 M, Rin 2.7 k i.e. maximum sustain, 1 kHz tone, input switched on for 1.5 s):**

| input level | gain reduction (dB, steady at 1.5 s) | attack to 63 % | attack to 90 % | release to 50 % of GR | LED current at end / +0.1 s / +1 s / +3 s |
|---|---|---|---|---|---|
| -45 dBV | 8.6 | 118 ms | 172 ms | greater than 5 s | 30 uA / 29 / 24 / 16 uA |
| -30 dBV | 22.4 | 41 ms | 44 ms | 1.9 s | 270 uA / 210 / 78 / 32 uA |
| -10 dBV | 40.2 | 40 ms | 43 ms | 0.59 s | 2.8 mA / 790 uA / 110 uA / 36 uA |

- **Attack** is set by C4 charging through R9 (about 40 ms when driven well above the knee, up to over 100 ms close to the knee); the LDR's own attack (a few ms) is faster. It is a **level-dependent attack**: harder notes compress faster.
- **Release** is set by C4 discharging into the LED. Since the LED current is exponential in its voltage, the discharge is **hyperbolic**: **I(t) = I0 / (1 + t / tau0)** with **tau0 = C4 x 0.05 V / I0** (0.41 s for I0 = 270 uA, 37 ms for I0 = 3 mA) (computed). Check: 270 uA after 1 s = 78 uA, matching the table. This gives a fast initial release after loud notes and a very slow tail (LED current 266 uA down to 25 uA takes about 4 s (computed)); the release time is **strongly level-dependent**: 50 % GR release is about 0.6 s after a -10 dBV input and about 1.9 s after a -30 dBV input; near the threshold it is longer than 5 s.
- LDR (VTL5C-class) own release (about 50 ms for the fast part, with the "memory" tail typical of these cells) adds to the above. The LDR's real curve is **assumed**; its tail is not modelled beyond the 50 ms constant.

**Static compression curve (computed, IC1B output level versus input level, both dBV rms; two LDR models)**: input -70 to -10 dBV in 5 dB steps:

| max sustain (Rin 2.7 k), model A | -70 | -60 | -50 | -40 | -30 | -20 | -10 |
|---|---|---|---|---|---|---|---|
| IC1B out dBV | -13.5 | -3.5 | 3.9 | 5.0 | 5.8 | 6.8 | 8.0 |

| max sustain, model B | -70 | -60 | -50 | -40 | -30 | -20 | -10 |
|---|---|---|---|---|---|---|---|
| IC1B out dBV | -21.9 | -11.9 | -1.9 | 2.5 | 3.7 | 4.7 | 5.7 |

| sustain 30 k series, model A | -70 | -60 | -50 | -40 | -30 | -20 | -10 |
|---|---|---|---|---|---|---|---|
| IC1B out dBV | -36.0 | -25.3 | -15.2 | -5.2 | 3.6 | 4.9 | 5.7 |

| sustain 100 k (min), model A | -70 | -60 | -50 | -40 | -30 | -20 | -10 |
|---|---|---|---|---|---|---|---|
| IC1B out dBV | -37.0 | -36.0 | -25.3 | -15.2 | -5.1 | 3.6 | 4.9 |

(The -37 dBV floor is simply the dark-LDR linear gain times the tiny input.) Reading the curves: the stage is linear below the detector knee, then **the output is held within about 4 dB over a 40 dB input range** above it: a **compression ratio of roughly 8:1 to 12:1** (model A: 4.1 dB out over 40 dB in = 10:1; model B: 3.9 dB over 35 dB = 9:1), with a **knee about 8 dB wide** (slope falls from 1.0 to about 0.1 between roughly -56 and -48 dBV input at max sustain, model A; -50 and -42 model B), set by the exponential LED / diode detector, so it is soft rather than a hard corner. **Threshold (input level where the knee occurs) = 2.0 V rms out / gain**: at max sustain **-52 dBV rms** (gain 815), at minimum sustain **-20.4 dBV rms** (gain 21.4) (computed), i.e. a **32 dB range** controlled by the sustain pot; if the LDR dark resistance is nearer 1 M than infinity the whole range shifts 10 dB up (-41.9 to -10.3 dBV rms). Output held at +4 to +8 dBV rms at IC1B before the clipper.

## 3. Soft clipper after IC1B

IC1B out, **R3 220 k** in series, node N. N sees to ground, with the diodes off, **R6 100 k + R8 27 k (+ C5 1 uF, 5.9 Hz corner)** = 127 k: small-signal attenuation 127 / (220 + 127) = **0.366 (-8.7 dB)** (computed). **Three 1N914 in series each way (D1, D3, D5 and D2, D4, D6, antiparallel)** are connected across R6: conduction starts at about 3 x 0.55 to 0.6 V = **1.65 to 1.8 V** at N (assumed 1N914 curve), i.e. at 4.5 to 4.9 V at IC1B out (peak): so at the held level (+4 to +8 dBV rms out of IC1B = 2.2 to 3.6 V peak, which is 0.8 to 1.3 V at N) the clipper is idle. It acts only on the **note-onset overshoot** (the 40 ms or longer attack lets the first peaks reach 7.4 V at IC1B, 2.7 V at N) and on very hot input. Above the knee the series 220 k and the stack's exponential I-V give a **soft, symmetric, 3-diode-per-side clip**, which limits N to about +/-2.0 to 2.5 V (assumed). N feeds the IC2A follower.

## 4. Wet path: IC2A and IC2B, blend

- IC2A TL074 unity buffer. IC2B inverting: Rin = R14 2.7 k + R15 100 k = 102.7 k, Rf = R19 68 k, so **gain = -0.662 (-3.6 dB)** (computed). **C9 2.2 nF** from the R14/R15 junction to ground with R14 2.7 k gives a low-pass at **26.8 kHz** (computed). Small-signal wet gain after IC1B: 0.366 x 0.662 = **0.242 (-12.3 dB)** (computed).
- BLEND 10 kB: dry (IC1A) at one end, wet (IC2B) at the other, wiper into R12 8.2 k then C7 680 pF to ground: low-pass 1/(2 pi x 8.2 k x 680 pF) = **28.5 kHz** (computed). Both ends are low-impedance, so the blend is a linear crossfade with no mid-position dip beyond the pot's own divider loss (-6 dB at centre: 0.5 each way) (computed).

## 5. Treble stage (IC2C) and output

- IC2C TL074, **non-inverting**: + input from the blend node (after R12), **TREBLE 100 kB** connected between that node (pin 1) and the inverting input (pin 3), wiper (pin 2) to **C8 1.5 nF in series with R20 2.7 k to VB** (AC ground), **R18 10 k** feedback.
- Wiper at the + end (pin 1): the leg only loads the input; gain 1 (0 dB) flat. Wiper at the - end (pin 3): the leg sets the gain: **gain = 1 + R18 / (R20 + 1/(j w C8))**: +0 dB below about 1 kHz (Z large), **+3.6 dB at 10 kHz and +7.3 dB at 20 kHz** (computed), with the C8 + R20 corner (R20 = Xc) at 39 kHz. So the control is **boost-only**, a gentle presence-style shelf, never a cut. (Flat at the other end; the intermediate positions blend.)
- C6 2.2 uF into **VOLUME 10 kA**: high-pass 1/(2 pi x 10 k x 2.2 uF) = **7.2 Hz** (computed); R17 150 ohm output resistor.
- Net level: a unity-gain follower into the blend, wet stage -12.3 dB small-signal, treble +0 to +3.6 dB at 10 kHz.

## 6. Supply (legible)

- **+9 V** (battery) with D12 1N4001 to ground (reverse clamp), **Q1 AO3401 P-MOSFET** reverse-polarity switch (gate to ground via R23 100 k with Z1 gate clamp), C15 100 uF, C13/C14 10 nF.
- **Two ICL7660** (IC3, IC5), each with **two 1N5817 Schottky diodes** (D13/D14, D15/D16) and 47 uF (C16, C17): outputs summed into **VA (C10 220 uF, C11 100 nF)**. Estimated VA = 2 x 9 V - 2 x about 0.3 V = **about 17.4 V** (computed; Schottky drop assumed, MOSFET drop assumed negligible).
- **VB = VA x 100 k / (91 k + 100 k) = 0.524 VA = about 9.1 V** (R21 91 k, R22 100 k, C12 100 uF) (computed). **IC4 LM741D** buffers it to **VC** (C18 470 uF) for the LED bridge reference.
- Indicator: **R10 20 k** from VA to the LED, "to switch".
- So the op-amps (NE5532, TL074) run on a single 17.4 V rail biased at 9.1 V: a swing of about +/-7.4 V (5.2 V rms, +14.4 dBV) at the rails with headroom assumed 1 V (computed). Sag: with only a 9 V battery and charge pumps the rail voltage will drop as the battery runs down; the IC1B output swing and the detector knee both depend on VA, VB. Not computed.

## 7. Compared with the plugin's assumptions (summary; details in Solid_and_Opto_vs_code.md)

The plugin's Opto Sustain was written from public descriptions of this class of pedal. This schematic gives the following hard numbers: threshold span about -52 to -20 dBV rms at the input (a 32 dB sweep), ratio near 10:1 with a knee about 8 dB wide, attack about 40 ms (level-dependent, up to >100 ms at the knee) set by a 2200 uF capacitor, a hyperbolic release (LED current 270 uA to 25 uA in 4 s; 50 % GR release 0.6 to 1.9 s depending on level), a first-order dark-state low-pass at about 2.2 kHz that opens with the LDR, a **soft 3-diode clipper on the wet path after the compressor**, and a **boost-only treble stage (+3.6 dB at 10 kHz, +7.3 dB at 20 kHz, 0 dB at the other end)**.

## 8. What could not be determined

- The VTL5C6's real resistance-versus-current curve, dark resistance and its fall/rise times (assumed models only; results ranged accordingly).
- Z1, the gate-clamp Zener in the supply (printed "unknown").
- The real 1N914 and LED I-V curves (assumed; they set the detector knee at about 2.9 V peak and the clipper at about 1.7 V).
- Whether the tracer's values (R7 2.2 M, C3 33 pF, C4 2200 uF, R9 47 ohm) match the production pedal.
