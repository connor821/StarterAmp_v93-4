# Boss CS-3 Compression Sustainer: circuit read

Source: `misc/CS3_p1.png` (service notes page 1: specifications, parts list, "AUG. 1986", "First Edition") and `misc/CS3_p2.png` (page 2: ET BOARD solder-side layout, VR BOARD, and the **circuit diagram**). I read the diagram at 2x to 4x; the print is clean. "(?)" marks anything I could not read or could not trace with confidence. Nothing here is applied to code.

## 0. Specifications and parts list (page 1)

- Power: 9 V battery or AC adaptor (BOSS ACA-100/120/220/240). Current draw 10 mA at 9 V DC.
- Equivalent input noise: -110 dBm (IHF-A). Input impedance 1 Mohm. Output load impedance: over 10 kohm.
- Controls: Level **50 kA** (EVJKAAP15A54, 50KA), Tone **20 kB** (EVJKAAP15B24), Attack **250 kC** (EVJKAAP15CF5, 250KC), Sustain **20 kB** (EVJKAAP15B24). The parts list gives "20KB: TONE, SUSTAIN".
- ICs: **uPC1252H2 = VCA** (the list calls it "VCA"); **M5218L** op amp; **BA718** op amp.
- Transistors: 2SC732TMGR; 2SC945P; FETs **2SK117-GR** and **2SK30ATM-Y** (the parts list says -Y; the diagram legend says 2SK30ATM-GR for Q4, Q5, so one of them is a typo or both grades are used).
- Diodes: 1SS-133 (switching), S5500G (rectifier), MTZ5.6-T77 or RD5.6EB3 (5.6 V zener), LED LN-28RP (red). Switch JM-0404 (foot switch). 
- Diagram legend: D1 = S5500G; D2, 3, 4, 5, 6, 7, 8, 10 = 1SS-133; D9 = MT25.6 or RD5.6EB3 (the zener); Q1 = 2SC732TMGR; Q2, 3, 6, 8 = 2SC945P; Q4, 5 = 2SK30ATM-GR; Q7 = 2SK117-GR; IC1, IC2 = M5218L; IC3 = uPC1252H2; IC4 = BA718.
- Boards: ET BOARD 7523252000 (pcb 2292037800) carries everything except the four pots; VR BOARD 7523254000 carries the pots.

## 1. Supply and bias (diagram, left edge)

- Jack pins: 18 = signal in, 13 / 16 / 17 = adaptor and battery lines (pin 17 ground). DC: **D1 (S5500G)** across the supply (polarity protection), **C1 100 uF / 16 V** reservoir. **R32 220 ohm + D10 (1SS133)** go from the battery/adaptor switch contact (pin 14) toward the 9 V line (battery isolation / adaptor detect; I did not trace it fully, (?)).
- **4.5 V reference (Vref):** **R28 10k** (from 9 V) and **R2 10k** (to ground) set the midpoint at IC1b's + input, with **C3 47 uF / 6.3 V** to ground; **IC1b (M5218L, pins 5, 6, 7)** is a unity-gain follower (pin 6 tied to pin 7) whose output (pin 7, "4.5 V") is the signal ground for the whole circuit. Up-arrows on the diagram are supply nodes; at pin 7 the arrow is labelled 4.5 V, and the "9V" arrow is the plain rail. Most other arrows are 4.5 V (bias and reference) but some are 9 V (IC pin 7 of IC3; Q1 collector; IC op-amp V+). I assign them by function and flag the ones I am unsure of.

## 2. Signal path in order

### 2.1 Input buffer
- Input (pin 18) -> **R1 10k** -> **C2 0.022 uF** -> IC1a (+) (pin 3). **R3 1M** from pin 3 to Vref sets the 1 Mohm input impedance (matches the spec).
- **IC1a (M5218L, pins 1, 2, 3)**: voltage follower (pin 2 joined to pin 1 output). **D2 and D3 (1SS133, antiparallel)** sit between the + input node and the output/- node: input clamp, limits the differential voltage to about +/-0.6 V to protect the op amp and tame large transients.
- Output (pin 1) feeds two places: **C4 1 uF/50 V** into the VCA, and a separate branch (a long wire to the top of the sheet) into **C14 1 uF/50 V** for the electronic bypass path (see 2.9).
- High-pass at the input: R3 and C2 = 7.2 Hz.

### 2.2 VCA (IC3, uPC1252H2)
- **C4 1 uF/50 V** -> **R9 10k** -> IC3 pin 1 (signal input).
- Pin 8 = output (current). Pin 7 = supply (up arrow, 9 V). Pins 3, 4 and 6 are tied together and to Vref (4.5 V). **Pin 5 is tied to ground through R7 2.7k.** **Pin 2** is the gain-control pin: it hangs on a node fed by **R10 1k** (to the reference) and **R8 47 ohm** (to the pin 4/Vref bus), and is pulled by **R11 15k** to the drain of **Q7 (2SK117-GR JFET)**; Q7's source goes to ground through **R13 10k**; Q7's gate is the envelope (control) node (see 3).
- I do not have the uPC1252H2 datasheet, so the pin functions above are as drawn, not as specified. The (?) flags: which of pins 3/4/6 is bias vs. reference.

### 2.3 Current-to-voltage stage (IC2a)
- IC3 pin 8 -> IC2a (M5218L) inverting input (pin 2). **R6 10k** feedback in parallel with **C5 0.0018 uF** (1.8 nF) from output pin 1 to pin 2; + input (pin 3) to Vref.
- Corner of R6/C5: 1/(2 pi 10k 1.8n) = 8.8 kHz (derived). Gain = 10k times the VCA's output current.
- Output (pin 1) goes to two places: **R5 10k** (to the tone stage) and the **detector branch** (the long vertical wire down the right of the VCA to the C6/R12 node, see 3).

### 2.4 Tone stage (IC2b)
- **R5 10k** -> IC2b + input (pin 5) and terminal 6 of the tone pot VR2.
- IC2b (M5218L, pins 5, 6, 7) is a non-inverting amplifier: **R4 10k** from the output (pin 7) to the - input (pin 6); the - input node is also terminal 7 of VR2.
- **VR2 20kB (TONE)** is connected between terminal 6 (the + input node) and terminal 7 (the - input node). Its wiper goes through **R1 4.7k** (note: the ET board's own R1 is the 10k at the input; this R1 4.7k label is on the VR board) and **C1 0.027 uF** to ground.
- Operation, derived: wiper toward the - input side puts the 4.7k + 0.027 uF leg across the feedback node, so the gain rises with frequency, up to 1 + 10k/4.7k = 3.1 (+9.9 dB) at high frequency; wiper toward the + input side shunts the + input instead and rolls the treble off. The leg's corner is 1/(2 pi 4.7k 0.027u) = 1.25 kHz (derived). So Tone is a treble boost/cut, 20 kB linear.
- Output (pin 7) -> **C13 0.047 uF** -> **R36 10k** -> terminal 5 of VR1 (Level).

### 2.5 Level control
- **VR1 50 kA (LEVEL)**: terminal 5 (top, from R36), terminal 10 (ground), terminal 4 (wiper). Wiper (terminal 4) -> Q4 drain (the FET switch, see 2.8).
- High-pass: C13 with R36 + pot: 0.047 uF into 10k + up to 50k: 56 Hz to 340 Hz depending on Level (derived).

### 2.6 Output buffer (Q1)
- After Q4: node -> **C15 0.047 uF** -> **Q1 (2SC732TMGR)** base. **R26 1M** from the Q4/C15 node to Vref (bias) and **R27 1M** from Q1 base to Vref (bias). Q1 is an emitter follower: collector to 9 V (up arrow), **R29 10k** from emitter to ground, emitter -> **C17 1 uF/50 V** -> **R31 1k** -> OUTPUT (pin 12), with **R30 100k** from the output node to ground (output load / click bleed). Jack output spec: load over 10k.

### 2.7 Pot terminal map (VR board)
Terminal numbers as printed on the diagram: 3 and 8 = ATTACK (VR3 250 kC), 2 = SUSTAIN (VR4 20 kB, with R4 4.7k and R3 22k on the VR board), 6 and 7 = TONE (VR2 20 kB), 5, 4 and 10 = LEVEL (VR1 50 kA), 9 = a supply tab. The VR board also carries **R1 4.7k, C1 0.027 uF, R2 22k, R3 22k, R4 4.7k** (the diagram prints these with the VR board labels; they repeat designators used on the ET board, so the same designator means two different parts, depending on the board).

### 2.8 Switching (foot switch, electronic bypass)
- **Foot switch** (pin 1) -> **R33 100 ohm** -> node with **C18 0.01 uF** to ground and **R34 1M** to the 9 V rail. The press pulls the node to ground, and this drives a bistable.
- **Flip-flop: Q2 and Q3 (2SC945P)**, each with a **56k collector load (R21 on Q2, R20 on Q3)**, a **100k cross-coupling (R22 from Q2 collector to Q3 base, R23 from Q3 collector to Q2 base)**, **470 pF speed-up caps (C11, C12)** across them, **56k base resistors to ground (R37 on Q2, R38 on Q3)**, and **220 pF trigger caps (C19, C20)** from the bases to the foot-switch node.
- Outputs: from the Q2 collector, **R24 1M -> D8 (1SS133) -> Q4 gate**, with **C16 0.047 uF** to ground (47 ms turn-on/off ramp, derived: 1M x 0.047 uF), and from the Q3 collector, **R19 1M -> D7 (1SS133) -> Q5 gate**, with **C10 0.047 uF** to ground. These gate networks slow the FET turn-on to suppress switching clicks.
- **Q4 (2SK30A)** is in series with the effect path: Level wiper -> Q4 -> C15 -> Q1. **Q5 (2SK30A)** is the bypass path: IC1a output -> **C14 1 uF/50 V** (with **R25 1M** to ground/Vref) -> Q5 -> same C15 node. Only one is on at a time.
- **LED** (LN-28RP, via **R35 3.3k** and the 5.6 V zener **D9**, "CHECK" jack pins 15 and 11) shows effect on and doubles as a battery check (it extinguishes when the supply falls below the zener threshold plus the LED drop).

## 3. Detector (side-chain) and envelope (the part that matters for modelling)

### 3.1 Tap and amplifier
- Tap point: the **IC2a output (VCA after the I-to-V stage)** goes down the long wire at the right of the VCA to the right end of **C6 1 uF/50 V** and **R12 47k** (the same node). So the detector listens to the compressed signal, after the VCA but before the tone stage: a **feedback (closed-loop) detector**.
- **C6 1 uF/50 V** -> **R15 100k** -> IC4a (BA718) - input (pin 3). **R16 100k** from the output (pin 2) to pin 3: an inverting amplifier of gain -R16/R15 = -1. + input (pin 4) to Vref. (BA718 pin numbers as drawn: 2 = output, 3 = -, 4 = +, 5 = ground.)
- Corner of C6/R15: 1.6 Hz.

### 3.2 Full-wave rectifier by two transistors
- **Positive arm (Q8, 2SC945P):** from the tap node, **R12 47k** -> **C7 0.01 uF** -> Q8 base. Base node: **R14 1M** to ground, **D5 (1SS133)** to ground (clamps the negative half), so the cap charges to the negative peak and the base sees only the positive half-cycle riding on a shifted level.
- **Negative arm (Q6, 2SC945P):** from IC4a output (pin 2) (the inverted copy), **R17 47k** -> **C9 0.01 uF** -> Q6 base node: **R18 1M** to ground, **D4 (1SS133)** to ground (the clamp). I could not trace what else connects at that node on the scan (the diagram shows C9 and the Q6 base on the node with R18 and D4), so the wiring of that arm is read as symmetrical to the Q8 arm (?).
- Q6 and Q8: emitters grounded, collectors joined to the **envelope (control) node**.
- High-pass corners (derived): R12 47k and C7 0.01 uF = 339 Hz; R17 47k and C9 0.01 uF = 339 Hz, so the detector ignores low fundamentals more than a flat detector would and favours picking attack.

### 3.3 Envelope node and the controls
- Envelope node: the Q6 and Q8 collectors, **C8 4.7 uF / 35 V** to ground (the reservoir), the gate of Q7 (the VCA control FET, see 2.2), **D6 (1SS133)** to IC4b, and the **ATTACK** terminal 8 (via the VR board).
- **IC4b (BA718, pins 6, 7, 8, 9):** voltage follower (output pin 8, - input pin 7 tied to the output); + input (pin 6) comes from terminal 2 of **SUSTAIN (VR4 20 kB)**. The VR4 network is: up-arrow supply -> **R4 4.7k** -> (node to pin 6) -> **VR4 20k (wiper tied to one end, used as a variable resistor)** -> **R3 22k** -> ground. So the reference at pin 6 is about V x (R_VR4 + 22k)/(4.7k + R_VR4 + 22k): from 0.82 V to 0.90 V per volt of the arrow's supply. If the arrow is 4.5 V that is 3.7 V to 4.05 V (derived; supply type of the arrow not certain, (?)).
- D6 couples the follower output to the envelope node as a diode (the direction on the print is not clear enough to call, (?)); function: it clamps (limits) the envelope node against the Sustain reference, so Sustain sets where the envelope can sit and with it the effective threshold or maximum gain change.
- **ATTACK (VR3 250 kC):** terminals 3 and 8; wiper tied to one end, so it is a variable resistor of 0 to 250k in series with **R2 22k** to the supply arrow. It is connected to the envelope node (terminal 8), so it is the resistor that recharges C8 4.7 uF between peaks. Derived time constants: **C8 x (22k to 272k) = 0.10 s to 1.28 s**. Whether this recharge path is what the panel calls "attack" (it is the speed at which the gain comes back, so it behaves as recovery/release on the gain) cannot be settled from the print; the knob is named ATTACK in the service notes (?).
- **Attack speed of the detector itself (derived estimate, not a spec):** a half-wave pulse of 1 V peak through R12 47k gives Q8 about (1 - 0.65)/47k = 7 uA of base current; at a gain of 100 to 300 that is 0.7 to 2 mA discharging C8 4.7 uF, about 150 to 450 V/s, so an overload of about 1 V is pulled down in around 2 to 7 ms. So the detector's own attack is a few ms.

## 4. Component list with values (ET board unless stated)

| Ref | Value | Where |
|---|---|---|
| R1 | 10k | input series |
| R2 | 10k | Vref divider, low side (ET); 22k on the VR board (attack) |
| R3 | 1M | input bias to Vref (ET); 22k on the VR board (sustain) |
| R4 | 10k (ET, tone stage feedback); 4.7k on the VR board (sustain) | |
| R5 | 10k | IC2a out to tone stage |
| R6 | 10k | IC2a feedback |
| R7 | 2.7k | IC3 pin 5 to ground |
| R8 | 47 ohm | IC3 control node to pin 4 bus |
| R9 | 10k | C4 to IC3 pin 1 |
| R10 | 1k | IC3 control node to reference |
| R11 | 15k | IC3 control node to Q7 drain |
| R12 | 47k | detector tap to C7 |
| R13 | 10k | Q7 source to ground |
| R14 | 1M | Q8 base to ground |
| R15 | 100k | C6 to IC4a pin 3 |
| R16 | 100k | IC4a feedback |
| R17 | 47k | IC4a out to C9 |
| R18 | 1M | Q6 base node to ground |
| R19 | 1M | flip-flop to Q5 gate (with D7, C10) |
| R20 | 56k | Q3 collector load |
| R21 | 56k | Q2 collector load |
| R22 | 100k | Q2 collector to Q3 base |
| R23 | 100k | Q3 collector to Q2 base |
| R24 | 1M | flip-flop to Q4 gate (with D8, C16) |
| R25 | 1M | bypass path bias (C14 / Q5) |
| R26 | 1M | Q4 / C15 node bias |
| R27 | 1M | Q1 base bias |
| R28 | 10k | Vref divider, high side |
| R29 | 10k | Q1 emitter |
| R30 | 100k | output to ground |
| R31 | 1k | output series |
| R32 | 220 ohm | supply / battery branch (with D10) |
| R33 | 100 ohm | foot switch series |
| R34 | 1M | foot switch node pull-up |
| R35 | 3.3k | LED series |
| R36 | 10k | C13 to Level pot |
| R37 | 56k | Q2 base to ground |
| R38 | 56k | Q3 base to ground |
| C1 | 100 uF / 16 V (ET); 0.027 uF on the VR board (tone) | |
| C2 | 0.022 uF | input |
| C3 | 47 uF / 6.3 V | Vref |
| C4 | 1 uF / 50 V | VCA input |
| C5 | 0.0018 uF | IC2a feedback |
| C6 | 1 uF / 50 V | detector tap |
| C7 | 0.01 uF | Q8 coupling |
| C8 | 4.7 uF / 35 V | envelope reservoir |
| C9 | 0.01 uF | Q6 coupling |
| C10 | 0.047 uF | Q5 gate |
| C11, C12 | 470 pF | flip-flop |
| C13 | 0.047 uF | tone out to Level |
| C14 | 1 uF / 50 V | bypass path |
| C15 | 0.047 uF | to Q1 base |
| C16 | 0.047 uF | Q4 gate |
| C17 | 1 uF / 50 V | output coupling |
| C18 | 0.01 uF | foot switch |
| C19, C20 | 220 pF | flip-flop triggers |
| VR1 | 50 kA | Level |
| VR2 | 20 kB | Tone |
| VR3 | 250 kC | Attack |
| VR4 | 20 kB | Sustain |

Printed value that I could not fully read: none of the R and C values above were unreadable at 2x to 4x. The things I cannot settle are connections, marked (?) in the text: the exact net feeding the Q6 arm, the direction of D6, the pin functions of the uPC1252H2, and which arrows are 4.5 V and which are 9 V.

## 5. What the circuit does (summary for modelling)

1. Input buffer with an input clamp, then the **uPC1252H2 VCA** (an IC with a current output) whose gain is set by a control current at pin 2, which is bled by the **JFET Q7 (2SK117)** from the envelope node. Output current converted by IC2a (10k, 8.8 kHz roll-off).
2. A **feedback** detector: IC2a output -> 1 uF -> inverting unity stage IC4a -> two transistors (Q6, Q8) rectifying both half-cycles into **C8 4.7 uF**. Detector high-pass about 339 Hz on both arms.
3. **Sustain** (20 kB) sets a reference through IC4b and D6 against the envelope node; **Attack** (250 kC with 22k) is a variable resistor in the recharge path of C8, giving 0.1 s to 1.3 s recharge constants (derived).
4. **Tone**: treble cut/boost around 1.25 kHz with up to +10 dB of HF gain, after the compressor. **Level** after the tone stage. Q1 emitter-follower output.
5. Electronic bypass: two 2SK30 FETs switched by a Q2/Q3 flip-flop with 47 ms gate ramps (anti-click); input buffer is always on, so there is no true bypass.
6. Specs: 9 V, 10 mA, 1 Mohm in, over 10 kohm load, -110 dBm EIN.
