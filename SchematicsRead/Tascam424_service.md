# TASCAM Portastudio 424 (mark one): service manual notes (channel, EQ, tape electronics)

Sources read
- The 17 scanned page images `tascam-20 .. tascam-36.pgm.png` (100 dpi). The files are not one page each: `tascam-25` (1700 px) is pages 25 and 26 side by side, `tascam-26` is pages 27 and 28, `tascam-29` is pages 31 and 32 (block diagram), and the four wide schematics (2300 x 1100) are one sheet each. Schematics were read in crops enlarged 3 to 10 times.
- The same service manual is `reference/tape/7f804a81-tascam-424-service-manual-and-schematics.pdf` (36 pages). **I also read its pages 2, 3, 12 to 19 (specifications and the amplifier adjustment tables), which were not in the file set**, because they give the figures the schematics cannot (tape speeds, bias frequency, response masks). Those are marked "(PDF p. n)".
- Reading quality: 100 dpi scans, component values legible but some digits ambiguous. Tags: (?) = unreadable or uncertain; "(derived)" = I computed it from the values read (my own network solutions, scripts not kept in the project).

## 1. Which page is what

| File | Content | Used for |
|---|---|---|
| tascam-20, 21 | Exploded view 1 and parts list (case, knobs, PCB assemblies). Parts list names the boards: IN PCB, OUT PCB, SW PCB, MIX PCB, BOTTOM PCB (+ shield sheets, heat sink, "TR 2SB891R" power transistor) | board names only |
| tascam-22, 23, 24 | Exploded view 2 and parts list: **mechanism** (R/P head "4-4" Permalloy, erase head "4-4", capstan/reel motors, belts, counter, solenoid) | nothing electrical; shows 4-channel R/P head and 4-channel erase head |
| tascam-25, 26 | PCB layouts: BOTTOM PCB (TRK 1 to 4 circuits side by side, 4 x AN7367K "7367", 4 x 4066), IN PCB, OUT PCB, MIX PCB, SW PCB | ref-des location only |
| tascam-27, 28 | **Parts lists**: BOTTOM, IN, OUT, MIX, SW PCBs (IC and transistor types, trimmer values, 87.5 kHz traps, 36 mH choke, bias transformer) | part types |
| **tascam-29** | **Block diagram** (4 mic/line channels with trim, input switch (mic/line or tape), 2-band EQ, input fader, pan, effect send, direct, tape feed; stereo input channels 5-6 and 7-8; bus, master, effect master, monitor/cue/phones; track-assign "record function" switches; 4-track 4-channel rec/repro amp with dbx Type II NR; 5-LED meters on TRK 1 to 4) | structure |
| **tascam-30** | **Level diagram** (dBV levels through the channel) | gain structure |
| tascam-31 | IC block diagrams (uPD7566CS microcontroller, LC7800 input expander, TC74HC4024 7-stage counter, IR2E31 5-dot meter driver, **AN7367K dual dbx noise reduction processor**, BA6109 motor driver) | ICs |
| tascam-32 | Back cover (addresses) | none |
| **tascam-33** | **Schematic: BOTTOM PCB (1/2), rec/repro amp section**: TRK 1 drawn in full, TRK 2 to 4 "same as TRK 1" (parts numbered 2xx, 3xx, 4xx), bias/erase oscillator driver, erase heads | tape electronics |
| tascam-34 | Schematic: BOTTOM PCB (2/2) control section (microcontroller, counter, motor, phones amp, regulators) and SW PCB | bias frequency source, phones |
| **tascam-35** | **Schematic: MIX PCB**: channel 1 in full (CH 2 to 4 "same as above"), stereo inputs, bus amps, master, effect master, monitor/cue, meter, record function | channel, EQ, bus |
| tascam-36 | Schematic: IN PCB, OUT PCB and the wiring diagram | input and output stages |

## 2. Specifications (PDF p. 2, 3, 12 to 19 unless noted)

| Item | Value |
|---|---|
| Tape | compact cassette C-30 to C-90, **high-bias (Type II, CrO2) only**; 4 tracks, single direction; 4-channel record/play Permalloy head + 4-channel ferrite erase head |
| Tape speed | **HIGH 9.5 cm/s (3-3/4 ips); NORM 4.8 cm/s (1-7/8 ips); SLOW 2.4 cm/s (15/16 ips)**; accuracy +/-1%; **pitch control +/-12%** |
| Wow and flutter | **0.05% WRMS, +/-1.0% W.Peak at HIGH; 0.07% WRMS, +/-1.2% W.Peak at NORM** |
| Noise reduction | dbx Type II (can be switched off; "off on ch.4 with the DBX switch at SYNC") |
| Overall record/play response | **40 Hz to 16 kHz +/-3 dB at HIGH; 40 Hz to 12.5 kHz +/-3 dB at NORM** |
| Tape S/N | HIGH 55 dB / 58 dB without dbx (UNWTD / IHF A-WTD), **90 / 95 dB with dbx**; NORM 54 / 56 dB without, 88 / 93 dB with |
| Tape THD | 1.0% or less at 1 kHz, 0 dB; the adjustment check (PDF p. 17) allows 2.0% or less at -10 dBV, HIGH, dbx out |
| Crosstalk (adjacent tracks) | 70 dB (1 kHz, 0 dB, with dbx); 50 dB without |
| Erasure | 70 dB or more |
| Mixer | mic/line input impedance **50 kohm**; nominal input **-50 dBV (3 mV) at trim max, -10 dBV (0.3 V) at trim min**; max input **+6 dBV (1.8 V) at trim min**; stereo inputs 10 kohm, nominal -10 dBV, **minimum input level -16 dBV** (sic); line outs 1 kohm, nominal -10 dBV; sync in 5 kohm; phones 8 ohm, 100 mW + 100 mW |
| EQ | **HIGH shelving 10 kHz +/-10 dB; LOW shelving 100 Hz +/-10 dB** (adjustment check: "+/-10 +/-2 dB", PDF p. 12) |
| Mixer response | **MIC IN or LINE IN to L/R line out: 20 Hz to 30 kHz +/-3 dB**; to phones 50 Hz to 20 kHz +/-3 dB |
| Mixer S/N | mic in to line out 66 dB at trim max; line in to line out 70 dB (DIN A, input shorted) |
| Mixer THD | 0.06% (1 kHz, 10 dB above nominal, mic) / 0.04% (line, nominal), 30 kHz LPF; adjustment check 0.05% or less |
| Crosstalk | 55 dB at 1 kHz, nominal level |
| Power | 12 V DC adaptor, 1100 mA, 18 W |

Alignment facts (PDF p. 14 to 19):
- **Bias oscillator frequency 87.5 kHz at the erase head terminals** (PDF p. 17). The schematic also prints "87.5kHz" on the net (tascam-33, tascam-34 connector pin H).
- Playback EQ trimmer R108 (R208 to R408): "0 dB at 10 kHz (same level as at 315 Hz)" on test tape MXT-1161; the playback response mask (Fig. 6-2): 63 Hz to 10 kHz with +/-4 dB below about 100 Hz tapering to 0 at 315 Hz, +/-3 dB from about 400 Hz to 10 kHz (NORM speed -4 dB lower limit at 10 kHz).
- Overall record/play mask, -30 dBV input (Fig. 6-3): **HIGH, dbx out: +3 / -3 dB from 63 Hz to 10 kHz, widening to +4 / -4 dB at 14 kHz; dbx in: +4 / -6 dB at the bottom, +4 / -4 to 10 kHz, +6 / -7 at 14 kHz. NORM, dbx out: +3 / -3 dB to 6.3 kHz, -4 at 10 kHz; dbx in: -6 dB at 10 kHz.**
- Bias set (R124, R224 to R424): equal output level for 1 kHz and 10 kHz recordings at -30 dBV (NORM speed, dbx in). Record reference level (R135): -10 +/-0.5 dBV off tape for a -10 dBV input (HIGH speed, dbx in). **dbx timing: 18.4 mV DC at pin 3 of R123 (R223 to R423) to ground.** Repro bias trap U102 (U202 to U402) adjusted for minimum bias leakage (-40 dB or less). Repro level R111: -10 dBV out from test tape MXT-112.
- Repro S/N check (leader tape): 48 dB or more at HIGH, 46 dB at NORM. Rec/repro S/N (dbx out): 46 dB HIGH, 44 dB NORM.

## 3. Channel input stage (MIX PCB, CH 1; CH 2 to 4 identical)

IN PCB (tascam-36): per input jack, **R101 1 kohm in series, R102 100 kohm to ground, C101 1500 pF to ground** (RF filter: 1 kohm with 1500 pF = pole at 106 kHz, derived). IN PCB jacks are 3-pole (tip, ring, sleeve).

MIX PCB (tascam-35, enlarged):
- C101 10 uF/25 V coupling; **R101 100 kohm to ground** on the board (with the 100 kohm on the IN PCB this gives the 50 kohm input impedance, derived, matching the spec).
- **U101a** (uPC4570HA / NJM4565-type dual op-amp) **non-inverting trim stage**: feedback **R102 470 kohm in parallel with C102 100 pF** from output to the inverting input; the inverting input also goes through **C103 10 uF/25 V** to the wiper of **R103, 10 kohm (RD) TRIM**. The pot is connected from the stage output (after **C104 10 uF/25 V**) through its track to ground via **R104 100 ohm**.
- **Derived gain**: G = 1 + (R103 top part || 470k) / (R103 bottom part + 100 ohm). At minimum trim (wiper at the output end) G = 1 (0 dB); at maximum trim (wiper at the 100 ohm end) G = 1 + (10k || 470k)/100 = 99 = **about 39.9 dB**. This reproduces the spec's 40 dB trim range (-50 dBV at trim max to -10 dBV at trim min) and the level diagram's "TRIM" arrow of 40 dB, so the topology reading is consistent.
- Because the trim wiper circuit has **C103 10 uF in series with the 100 ohm leg, at maximum trim the low end rolls off**: corner 1/(2 pi x 100 ohm x 10 uF) = 159 Hz, so at full gain the stage is -2.9 dB at 159 Hz and about -7 dB at 80 Hz relative to its mid-band 40 dB (derived; at lower trim settings the series resistance is larger and the corner falls below 20 Hz). At 0 to 20 dB trim this effect is negligible.
- Noise/clip: U101 sits on the board's analog supply (the sheet labels +10 V, +5 V and "A. +5V" with "A. GND"; the rec/repro connector also carries -5 V). Which op-amps run on which rail is not verified here (?).
- **Input select switch S101** (3 positions, A / B / C): A = tape and mic/line (CH L or R), B = tape, C = mic/line (the notes under the switch read "A: TAPE, MIC/LINE=L (?); B: TAPE; C: MIC/LINE"). Block diagram: "INPUT: MIC/LINE or TAPE" then EQ.
- The stage after the EQ: **input fader R113 10 kohm (A taper, "10k (A)")**, **C111 10 uF/25 V**, **R118 47 kohm** into **U102a** inverting amplifier: feedback **R116 47 kohm (?) in parallel with C112 10 pF** (so unity gain, inverting, with a pole at 1/(2 pi x 47k x 10 pF) = 339 kHz, derived; resistor digits slightly uncertain: "47k" read for both R116 and R118). Output **C113 10 uF/25 V**.
- **EFFECT send**: R122 10 kohm (A), then R123 47 kohm to the effect bus; **DIRECT** (a tap before the pan) goes to the track-assign switches; **TAPE** is the return from the tape tracks.
- **PAN**: R119 10 kohm (B), centre click (CC). Circuit: C113 output feeds **R117 4.7 kohm to the L node and R118 4.7 kohm to the R node**; the pan pot lies between the L node and the R node with its **wiper to ground**; **R120 75 kohm (L) and R121 75 kohm (R)** feed the bus. Derived pan law (pot linear, bus summing node at virtual ground): hard-left node voltage = 10k||75k/(4.7k + 10k||75k) = 0.65 (-3.7 dB), centre = 5k||75k/(4.7k + 5k||75k) = 0.50 (-6.0 dB), so **centre is about 2.3 dB below hard pan** (a -2.3 dB centre pan law, derived).
- **Bus amps** (U105 a/b): C117 10 uF in; inverting, **R135 120 kohm || C118 10 pF** feedback; per channel the 75 kohm into the virtual ground gives 120k/75k = 1.6 = +4.1 dB at hard pan (derived), feedback pole 133 kHz. Master: **R137 10 kohm (A) x 2**, **R138 47 kohm** into **U106 a/b**, feedback **R139 120 kohm || C121 10 pF**: **maximum master gain = 120k/47k = 2.55 = +8.1 dB** (derived), pole 133 kHz. Effect master: the same structure (10 kohm (A) pot, 47 kohm, 120 kohm || 10 pF; reference numbers not read).
- **Level diagram (tascam-30)**: line input -10 dBV at the jack and through the channel; mic input -50 dBV; **trim arrow spans 40 dB**; **EQ LOW (100 Hz) +/-10 dB, HIGH (10 kHz) +/-10 dB**; **the input fader step reads "(-16)"** (a 6 dB drop from the -10 dBV line, then regained at the following stage); master and effect-master steps also drop to "(-16)" and return to **-10 dBV at LINE OUT and EFFECT SEND**; STEREO INPUTS enter at -10 dBV and drop to -16 at the LINE (level) control. 0 dBV = 1.0 Vrms. The exact fader/master nominal positions are not given beyond these labels.

## 4. Channel EQ network (MIX PCB CH 1, U102b)

Active **Baxandall-type shelving tone control around the second half of U102** (op-amp "U102b", inverting input as summing node, non-inverting input to A.GND).

Components read (enlarged crops, digits mostly clear):
- Signal comes from the input switch (node "IN", also loaded by **R114 10 kohm** to ground).
- **HIGH: R107 100 kohm (B) CC "EQ HIGH (10 kHz)"** connected as the series element between IN (pin 3) and the output-side rail T (pin 1); wiper (pin 2) goes through **C105 0.0010 uF (1 nF)** and **R109 10 kohm** to the summing node X.
- **LOW: R108 100 kohm (B) CC "EQ LOW (100 Hz)"**: IN goes through **R106 22 kohm** to pin 3 of R108 (node B3); pin 1 (node B1) is tied to rail T through **R111 18 kohm**; **C106 0.016 uF (digits read 0.016 or 0.015 (?)) from B3 to the wiper** and **C107 0.015 uF from the wiper to B1**; the wiper goes through **C108 10 uF/25 V** and **R110 5.6 kohm** to X.
- Output side: **R112 470 kohm || C109 47 pF** from the op-amp output to X; **C110 10 uF/25 V** from the output to rail T (so the pots' far ends connect to the output through a DC-blocking cap).
- Output goes to the input fader.

Derived response (nodal solution of exactly this network, linear pots, centre = flat):

| Quantity | Result |
|---|---|
| Flat-position gain | -1.8 dB, essentially flat (+/-0.2 dB) from 20 Hz to 20 kHz (inverting) |
| LOW boost at 100 Hz | **+11.9 dB** (plateau +13.9 dB at 20 Hz), half-gain frequency about **265 Hz** (shelf corner) |
| LOW cut at 100 Hz | **-12.6 dB** (plateau -15.1 dB at 20 Hz), half-gain about 260 Hz |
| HIGH boost at 10 kHz | **+7.5 dB** (+10.3 dB at 20 kHz, +11 dB plateau), half-gain about **6.8 kHz** |
| HIGH cut at 10 kHz | **-7.3 dB** (-10.1 dB at 20 kHz) |
| Effect at 1 kHz | LOW +0.8 / -1.2 dB; HIGH +0.7 / -0.8 dB (the shelves are slightly broad) |

So the derived response agrees with the stated "+/-10 dB at 100 Hz and 10 kHz" (service check +/-10 +/-2 dB) within about 2 dB at 100 Hz (+12) and 2.5 dB at 10 kHz (+7.5: just under the 8 dB lower tolerance, so either the HIGH shelf corner is a little lower than I derived, or the "B" pot taper and tolerances move it). The shelving character, broad (starts about 4 octaves wide) and rising to about +11 dB / -15 dB at the extremes, is certain; the exact corner frequencies are +/- 30%.

## 5. Tape electronics (BOTTOM PCB 1/2, TRK 1; TRK 2 to 4 identical)

### 5.1 Playback (head to dbx)

- **Head** (Permalloy R/P head): P1 contacts 1 (play/record hot), 2 (common/ground), 3 (record side, receives bias). Play path from contact 1: **R101 220 ohm** series, then the **87.5 kHz bias trap U102** (a parallel L-C coil trap, parts list "COIL, TRAP 87.5KHZ", tuned by the trimmer-like adjustment "U102"), **C102 10 uF/25 V** coupling, **R105 100 kohm to ground**. **Q101 (2SD1450S/T)** with base network **R102 10 k / R103 10 k** and **C101 0.022 uF**, controlled from the 5-pin block **U101 (BA7755; pins 1 to 5, R104 33 kohm on pin 5)** shorts the head line to ground (the record-mode mute of the playback preamp (?); role inferred from placement).
- **U103a head preamp** (UPC4570HA): non-inverting; input **C102 10 uF** with **R105 100 kohm** to ground; gain leg **R106 100 ohm + C103 100 uF/10 V** to ground; feedback network (see below).
- **Playback equalisation network** (the "REPRO EQ ADJ"): from the inverting input: **R107 330 kohm in parallel with C104 0.01 uF** in series with **R108 4.7 kohm (LB) trimmer (schematic text; the parts list says 6.8 KB for R108 to R408: conflict, one is misread (?))**, in series with **R109 3.9 kohm**, to the output. A 4066 switch **U112b** (control pin 5) is connected in parallel with R109 (pins 4 and 3), so closing it shorts R109 out.
  - **Low-frequency time constant: R107 x C104 = 330 kohm x 0.01 uF = 3.3 ms** (corner 48 Hz); the NAB cassette/reel playback LF time constant is 3180 us (50 Hz). Within about 4% of NAB.
  - **High-frequency time constant** (derived): tau_HF = (R108' + R109) x C104 x R107/(R107 + R108' + R109) with R108' the trimmer's setting: with R109 in circuit (switch open) and a 4.7 kohm trimmer: **39 us (R108' = 0) to 84 us (4.7k)**, centre 61 us; with the 6.8 kohm reading: 39 to 104 us. **Target is 70 us (IEC Type II, corner 2.27 kHz)**, which is near the trimmer's centre. 120 us (Type I, 1.33 kHz) is not reached with the 4.7 kohm reading. With U112b closed (R109 shorted) the range is 0 to 46 us (R108 = 4.7k) which would be an equalisation for a faster/other state (the control line is not traced, so which speed or mode closes it is **unknown (?)**).
  - Derived playback gain (R108 at centre, switch open): +13.8 dB at 30 Hz, +9.0 dB at 100 Hz, 0 dB at 315 Hz (reference), -9.4 dB at 1 kHz, -15.8 dB at 3 kHz, **-17.9 dB at 10 kHz** relative to 315 Hz (the head's own +6 dB/octave rise makes the combination flat: 315 Hz to 10 kHz is +30 dB of head slope against -17.9 dB of EQ). Mid-band gain of the preamp is about **+45 dB at 1 kHz and +54 dB at 315 Hz** (derived).
- After the preamp: **C105 (1 uF/50 V (?))**, **R110 3.3 kohm (?)**, then **U104 "L.P.F. 20 kHz"** (a prefabricated 4-pin LC low-pass block with two caps and two inductors; parts list "L.P.F., 20KHZ"), **Q105 buffer (2SD1450S/T)**, **R111 10 kohm (B) "REPRO LEVEL ADJ"**, then analog switch **U112c**, **R112 18 kohm (?)** and **U105a** (NJM4565LD inverting stage with **C108 400 pF** feedback and **R113 25 kohm (?)**), **C109 10 uF/25 V** into the dbx IC **U106 (AN7367K) pin 2** (signal input). Tape out/sync buffers: **Q104 (emitter follower) and U105b (R141 36 kohm || C140 47 pF (?), C139 2.2 uF/50 V, R140 9.1 kohm (?)), C141 10 uF/25 V** to the tape-out bus.
- dbx support parts read around U106/U107/U108 (not all readable): **C111 300 pF (PP), C112 2.2 uF/50 V, C113 0.001 uF (CE), C114 10 uF/16 V (BP), C115 47 uF/16 V (BP), C116 10 uF/25 V, C117 0.001, C118 0.1, C119 0.1, C120 0.0033, C121 330 pF (PP), C122 0.0033; resistor arrays U107 "5012" (47k, 24k, 6.2k visible) and U108 "D002" (33k, 4.7k, 9.1k, 33k, 6.8k visible); R120 1k, R121 1k, R123 trimmer 4.7 kohm (dbx timing, 18.4 mV)**. The exact values per node cannot be matched to the circuit reliably (?).
- **dbx Type II** (AN7367K, "dual dbx noise reduction processor", pins: signal input, emphasis, line amp output, level sensor, rec amp output, CCA etc.): it is a 2:1 compander in record and 1:2 expander in play, with an RMS level sensor and pre/de-emphasis. The service manual's alignment (dbx timing 18.4 mV, response with dbx in and out) matches that; the manual itself gives no ratio or time constants (the compander parameters are general dbx Type II knowledge, not from these pages).

### 5.2 Record and bias

- **Bias source**: microcontroller **U502 (uPD7566CS-180)** with ceramic resonator **X501 CSB700A (700 kHz)** and counter **U503 TC74HC4024AP (7-stage binary counter, pin 9 = Q3 = divide by 8)**: **700 kHz / 8 = 87.5 kHz**, the net labelled "87.5kHz" (derived and confirmed by the label and by the service manual's 87.5 kHz check).
- **Bias/erase driver** (schematic bottom-left): TC4049BP (hex inverter "U2"): four gate buffers **U2c/d/e/f** drive the four erase heads through **C1 to C4 0.01 uF + R2 to R5 1.8 kohm**, with **R7 to R10 2.7 kohm** to ground; **U2a/b** with **D2, D3, C5, C6 0.022 uF, R12 and R13 10 kohm** gate the 87.5 kHz; transistor **U1 (JC501Q) emitter follower with R14 10 ohm and D4**; large decoupling banks (47 uF/16 V x 8, 0.022 uF x 16). Bias is amplitude-controlled by the "BIAS CONT" lines (control logic from the microcontroller / record-function switches).
- **Per-track bias transformer T101 (P-ST424)**, driven by **Q106 (2SC2062C)** and **Q102 (2SC2002L)** (with R127 3.9 ohm emitter, C142 10 uF/25 V, D108 diode (?), C107 10 uF/25 V, C128 47 uF/16 V, R126 10 kohm, R149 4.7 kohm, L102 10 uH choke, R148 1.5 kohm, R145 220 ohm, D107, C129 22 uF/25 V). **Pin 3 of T101 goes through R124 100 kohm (B) "BIAS ADJ" in series with C126 100 pF (PP) to head contact 3**: at 87.5 kHz the 100 pF is 18.2 kohm of reactance, so **bias reaches the head as a current set by the secondary voltage and (18 kohm combined with 0 to 100 kohm) of series impedance**, i.e. constant-current-like bias. **C127 0.0082 uF (PP)** from T101 pin 2 to ground (87.5 kHz tuning of the secondary (?)). The link of pin 2 and of the record-signal feed to the head is **not fully traced (?)**.
- **Record amplifier** (U103b): input from the record chain through **C130 10 uF/25 V** and **R125 10 kohm** (with a second **87.5 kHz trap U109**, parts list "TRAP 87.5KHZ", in the line ahead of R125); feedback is two series sections: **(R129 150 kohm || C131 0.1 uF) in series with (R130 33 kohm || C132 220 pF (CE))**. Derived: first section corner = 1/(2 pi x 150k x 0.1 uF) = **10.6 Hz** (below it the gain is 150k + 33k = 183k/10k, above it 33k/10k); second section corner = 1/(2 pi x 33k x 220 pF) = **21.9 kHz**. **Mid-band gain = 33k/10k = 3.3 = +10.4 dB**, with a rise of about +4.4 dB below about 10 Hz (inaudible) and a fall above 22 kHz: **no recording pre-emphasis in this amplifier**; the record HF boost must come after it.
- After U103b: **C133 10 uF/25 V** to a node with **C134 0.0068 uF and R132 2.2 kohm** to ground, then **L101 36 mH (parts list "CHOKE 36MH")** in series with **R131 1 kohm** and **C135 0.0056 uF**, a node with **C136 0.0055 uF and R133 100 kohm** to ground, and the 4066 switch **U112a** to ground (control pin 13/15 from the record function) (?). **L101 36 mH with C135/C136 about 5.5 nF resonates at 1/(2 pi sqrt(36 mH x 5.5 nF)) = 11.3 kHz; with 6.8 nF (C134) 10.2 kHz** (derived). This is the record equalisation / bias-trap network feeding the head; **its topology (where it enters the head circuit) could not be traced at this resolution (?)**, so the record-current response is not derived.
- Record level: **R135 10 kohm (B) "REC LEVEL ADJ"**, **R118 5.6 kohm (?)**, **Q103** (emitter follower, 2SD1450S/T), **R136 22 kohm**, **R116 47 kohm**, **U111 (DTA114ES) gate**, **R134 100 kohm**, **C137 10 uF/25 V**, **Q104 + R137 10 kohm / R146 22 kohm / R138 10 kohm / R150 220 ohm, D101 to D103 (MA165P), C138 22 uF/25 V** (limiter/clamp diodes in the record feed); these are record gating, level and protection parts.

### 5.3 Other tape-related items

- **Erase**: 87.5 kHz drive to four erase heads (see above); erasure spec 70 dB.
- **Phones amp**: U801a/b LA6515 with feedback resistors and output caps whose values I could not read reliably (?).
- **Transport**: reels and capstan DC servo (BA6109 motor driver, uPD7566), "HIGH SPEED / NORMAL SPEED / pitch" lines from the MIX PCB switch S4 (TAPE SPEED: HIGH, NORM, SLOW) and the PITCH CONTROL pot to the microcontroller (values not read).
- **Meters**: IR2E31 5-dot LED driver per track; the LEDs read about **+6, +3, 0, -5, -10 dB** (the scale on the block diagram; the reference level is not stated (?)).

## 6. What you can derive for modelling the 424 sound

| Property | Value | Basis |
|---|---|---|
| Mixer channel top-end | **flat to 30 kHz** (-3 dB at 30 kHz), bottom flat to 20 Hz (+/-3 dB) | spec (PDF p. 2) |
| Channel EQ | LOW shelf 100 Hz +/-10 dB (derived corner about 265 Hz), HIGH shelf 10 kHz +/-10 dB (derived half-gain about 6.8 kHz); derived +/-12 dB at 100 Hz, +/-7.5 dB at 10 kHz | spec + derived |
| Gain structure | mic -50 dBV to line -10 dBV via 40 dB trim; unity-gain stages after; fader/pan/bus/master add about +4 dB bus, up to +8 dB master | spec, level diagram, derived |
| Channel noise/distortion | 66 / 70 dB S/N, 0.04 to 0.06% THD (very clean) | spec |
| **Tape playback EQ** | 3.3 ms LF (48 Hz), HF 39 to 84 us (4.7k trimmer; 39 to 104 us if 6.8k), 70 us target, **head signal EQ'd by about +14 dB at 30 Hz and about -18 dB at 10 kHz relative to 315 Hz** | derived |
| **Overall response** | **40 Hz to 16 kHz +/-3 dB (HIGH 9.5 cm/s), 40 Hz to 12.5 kHz (NORM 4.8 cm/s)**; masks +3/-3 dB (dbx out) to 10 kHz | spec + Fig. 6-3 |
| **Top roll-off** | not produced by the electronics (the preamp's networks as read have no pole inside the audio band apart from the playback EQ; the 20 kHz LPF block and 87.5 kHz traps sit above it): comes from tape/head gap loss, record EQ, and the response masks above; at 14 kHz the HIGH-speed mask is -4 dB (dbx out) | derived |
| **Head bump** | **not in any schematic**; the playback EQ's +14 dB at 30 Hz is the LF boost (3.3 ms), the bump itself is a head-contour effect whose frequency scales with tape speed (so it is 2x higher at 9.5 cm/s than at 4.8 cm/s); the 40 Hz low end of the response spec is the only figure | not derivable (?) |
| **Bias** | 87.5 kHz (700 kHz / 8), current-fed through 100 pF + 0 to 100 kohm | schematic + PDF p. 17 |
| **Flutter and pitch** | 0.05% WRMS (HIGH), 0.07% (NORM), peak +/-1.0 / 1.2%; +/-12% pitch control | spec |
| **Noise** | 55 to 58 dB without dbx, 88 to 95 dB with dbx | spec |
| **Compander** | dbx Type II, switchable on/off; the spec says it is off on channel 4 when the DBX switch is at SYNC | spec, block diagram |
| Tape level | -10 dBV in = -10 +/-0.5 dBV off tape (calibrated), distortion check 2.0% max at -10 dBV; overall THD 1.0% at 0 dB | PDF p. 3, 17 |

## 7. Unreadable or unresolved (marked (?) above)

- Topology of the record-signal path into the head (where L101/C135/C136/U112a feed the head, what T101 pin 2 connects to); record EQ values; U101 (BA7755) function.
- The trimmer R108: 4.7 kohm (schematic) versus 6.8 kohm (parts list).
- Some resistor/capacitor digits: R110, R112, R113, R116, R118, C105, C106 (0.015 or 0.016), R140/R141, R148/R149 region, dbx array values.
- Which condition closes U112b (shortens the playback HF time constant): not traced.
- The head's inductance and the bias level are not given in any of the pages read.
