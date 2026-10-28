# UREI 1176LN schematic notes (Rev K, S/N suffix "K") plus manual facts

Sources read
- `misc/1176sch.png` (2800 x 1296): UREI 1176LN, "Effective for S/N suffix K", drawn 3-15-73, drawing P119147 (?), revision block up to Rev. K / N ("Rev G change circuitry, 1176LN suffix ' ' becomes suffix G, 3-24-79"). "STATIC DC VOLTAGES ADDED 23-10-02 J.E.P." - the bold voltages on the drawing are a later hand/CAD addition. This is the same drawing as the Rev K sheet on p-57 of the manual set (which is only 1796 px wide and too soft to read); the 2800 px copy was read in 2x tiles.
- `urei/p-001..p-060` (the files are actually named `p-01.png` .. `p-60.png`): UREI 1176LN manual (S/N 7652 onward), JBL/UREI factory test sheet (12/10/90), calibration notes, hand-written factory notes, revision log, schematics Original and Rev A to H, K.
- Reading quality: component values below were read at 2x; things I could not read with confidence are marked (?). "(derived)" means I worked it out from the drawing, not printed on it.

Not in the manual: there is no parts list / BOM for the 1176LN itself (only a BOM for the 1176SA stereo adapter on p-44), no input/output transformer winding data (except the power transformer 11754 on p-58), no FET data.

## 1. Block diagram (Rev K)

```
XLR/terminal strip TS-1
  -> IC2-A/IC2-B (5532) differential input amp, gain ~0 dB, 20k bal / 10k unbal
  -> INPUT pot R5 10k (front panel)
  -> R6 27k series -----+---> node "A" ---> C1 1.0u -> R10 1k -> Q2
                         |
                      Q1 BF245A (drain)  <- gate = GR bus (negative bias, rises toward 0 V on gain reduction)
  -> SIGNAL PRE-AMP  Q2, Q3, Q4 (BC107/2N3391A), with feedback R21 7.5k
       +-> C9 47u -> OUTPUT pot R27 100k -> C10 1.0 -> SIGNAL LINE AMP Q5..Q9 -> C15 470u -> T2 (Lundahl LL5402) -> X/Y output
       +-> R26 56k -> ratio-select divider (S1B..S4B, R2/R24/R25/R23) -> S9 -> C17 1.0u -> GR CONTROL AMP Q12..Q15
              -> C19/C20 6.8u -> CR3/CR4 (FD333) full-wave rectifier -> attack R76/R77/C22 -> GR bus -> Q1 gate
  GR bus -> R48 3.9M -> Q10 (BF245A) -> IC1 TL071 -> meter (GR mode)
```

Topology: the sidechain is taken AFTER the gain-reduction FET (the FET is ahead of the preamp, the control amp listens to the preamp output) so it is a feedback compressor. The sidechain tap is before the OUTPUT pot, so the Output knob does not change what is detected; the Input knob does (it sits ahead of the FET).

## 2. Input stage (Rev K)

| Item | Value |
|---|---|
| Connector | TS-1 barrier strip: OUTPUT +, COM; GRD; INPUT +, COM. 100 pF to ground on each input leg (two 100 pF caps, RF) |
| Input amp | IC2-A and IC2-B, marked "5532" (NE5532 dual). R1-A/B/C/D/E = five 10k resistors (matched network "R1"), R3 100k + R4 100k from +30 V to ground (mid-rail bias, C3' 47/40 decoupling, "CY 47/40"), C30 6.8/35 V and C31 6.8/35 V input coupling, C32 220 pF across feedback R1-A, C33 6.8/35 V output coupling, C35 0.1 supply bypass. Differential, gain about unity (derived) |
| Input impedance | 20 kohm balanced, 10 kohm unbalanced (manual p-05); test: two 10k 1 % source resistors give a 6.0 dB drop (+/-0.25 dB) |
| CMRR | typically better than 60 dB (theory page); factory test: more than 70 dB below 2 kHz reference at 60 Hz and 20 kHz, trimpot R2 (R2 in the older boards) |
| Input pot R5 | 10k, front panel "INPUT (F.P.)", feeds R6 27k |
| Old revisions | Original/A/B: balanced input transformer T1 (UTC O-12 or equivalent, 600 ohm) with a "T" attenuator (R1A, R2, R3, pot R18, values (?)), R4 270 across the secondary, then R5 27k. Rev G replaced the input transformer by the IC differential amp (revision notes: "G: bridging input, threshold lower by 6 dB, lower noise with input control below 3/4, IC voltage regulator") |

## 3. Gain-reduction FET attenuator (VVR)

- Series element R6 = 27k (Rev D and Rev K both 27k). Shunt element Q1 = N-channel JFET, "URET 13-0027", marked **BF245A** on Rev K (original part: 2N5457, later 13-0027; Rev D 1239-2331 changed to 13-0027). Note 4 on the drawing: "Q1 and Q10 are selected". Q1 is a socketed, selected part: units "with Q1 removed" still pass signal (test p-34 uses this).
- Drain of Q1 at the junction R6 / C1. Source goes to the bias/bypass node (R8 10k to ground, C2 100u/25 V; Rev D: C28 50u/6.4 V and R10 10k). The exact AC path of C2 (to ground vs to the Q2 emitter tap) is (?) - it looks like it ties to the R13/R14 emitter-ladder junction of Q2.
- Gate of Q1 = the "GR bus" (wire colour WHT, from the attack control). Resting gate voltage is set by the Q-bias network (section 6) to a small negative value (hand note: voltage on the wiper of R81 should read -1.5 to -2 V) so the FET is just pinched off and a hair into conduction; GR meter and manual say "this places the gain reduction FET Q1 slightly into conduction".
- Operation (manual 4.2): "L" section, R6 series, Q1 voltage-variable shunt resistor. "Below the threshold the VVR has a high resistance due to its quiescent bias. Starting at the threshold point the bias is reduced, causing the VVR resistance to decrease. The rate of change of bias, as well as the threshold point, is controlled by the Compression Ratio pushbutton switches."
- Linearising network at the gate: R9 2.2M in parallel with C3 200 pF and C4 200 pF, R7 2.2M, C27 0.22 uF, C5 0.22 uF coupling from the preamp output through R21 7.5k, R16 100 ohm trimpot ("signal preamp linearity", R15 150 ohm). R16 "is in the feedback loop of the amplifier and affects the operation of Q1" (manual 5.3.5): adjust for minimum THD at 500 Hz, -30 dB in, GR mode, Output at "18", 20:1. Hand note p-42: "500 Hz -10 dB in, 10 dB limiting 20:1, +10 dB output, with R16 adjust DIST to minimum: was .25 %, now ~.03 % (?), .15 % (?)". This looks like the classic scheme of feeding part of the drain signal back to the gate to cancel the FET's square-law distortion (derived, (?)).
- Note on Q1 contamination test (p-38): hot breath on Q1 must not change level (flux leakage on the high-impedance gate).
- Attenuation range (derived): R_ds(Q1) ranges from several hundred kohm or more (pinched off) to a few hundred ohms (Vgs near 0), so the L pad can attenuate from ~0 dB to well over 40 dB; the front-panel meter goes to about -20 dB.

## 4. Compression-ratio buttons (S1..S4, Rev K naming)

The four interlocking buttons are 4:1 (S1), 8:1 (S2), 12:1 (S3), 20:1 (S4); the "all buttons in" state is the extra (British mode). (Rev D drawing labels the switch banks "4:1 / 8:1 / 12:1 / 20:1" above S1..S4 in the same order.) Each button has two sections: B section = sidechain level divider, A section = rectifier bias ladder. Switches are drawn "normally open".

### 4a. B sections: sidechain level (signal divider into the control amp)

Series chain from the preamp output (right to left): **R26 56k - (node X, UA4706 "T+C" block, factory selected value *) - S4B tap - R25 68k - S3B tap - R24 56k - S2B tap - R2 56k - S1B tap - R23 47k to ground (via S9 black lead)**. All B-switch outputs go to one rail that feeds the control amp input through S9 (the "GR disable" switch, located on the attack pot R77) and C17 1.0u/100 V. Total series path = 56+68+56+56+47 = 283k in every ratio (constant input impedance).

Divider ratio into a high-impedance load (derived, ignoring the UA4706 block, base current and R60/R61):

| Button | Tap | Series R | Shunt R to ground | Voltage ratio | dB |
|---|---|---|---|---|---|
| 4:1 (S1B) | after R2 | 236k (R26+R25+R24+R2) | R23 47k | 0.166 | -15.6 |
| 8:1 (S2B) | after R24 | 180k | R2+R23 = 103k | 0.364 | -8.8 |
| 12:1 (S3B) | after R25 | 124k | 159k | 0.562 | -5.0 |
| 20:1 (S4B) | after R26 | 56k | 227k | 0.802 | -1.9 |
| all four in | all taps shorted | 56k | 47k | 0.456 | -6.8 |

Ratios of these voltages vs 4:1: 20:1/4:1 = 4.8, 12:1/4:1 = 3.4, 8:1/4:1 = 2.2, close to 5, 3, 2 - so the ratio is essentially proportional to the sidechain gain. All-buttons = 2.75 x the 4:1 level (an effective figure near 11:1 (?)), but with the A-bank bias of the low-threshold buttons (below); the UA4706 block is unread (?).
The block marked "UA4706 T+C *" (Rev D: same block, arrow "*") is a factory-selected value between the divider top and the rail (?). It is not documented elsewhere in the manual.

### 4b. A sections: diode bias ladder (threshold)

Ladder: top node (left end) - R87 150 ohm (to the GR bus/release-network node, ~-1.7 V) ; R46 10M across the whole bank (Rev E addition "10M resistor on ratio switch", keeps the node from floating between buttons); series R42 470 (1/4 W), R43 560, R44 1.5k between taps S1A-S2A, S2A-S3A, S3A-S4A; right end = node "VIO"(20) with R45 2.2k to -10 V. The common lower rail (wire GRAY, terminal 21) feeds the junction of R74 47k / R75 47k that bias the rectifier diodes CR3/CR4 (and C21 100u/25 V bypass).
Bias voltage at the GRAY node (derived, taking the GR-bus rest voltage as -1.7 V):

| Button | Resistance toward bus node | Resistance toward -10 V | GRAY voltage |
|---|---|---|---|
| 4:1 (S1A) | 150 | 470+560+1500+2200 = 4730 | about -1.95 V |
| 8:1 (S2A) | 620 | 4260 | about -2.75 V |
| 12:1 (S3A) | 1180 | 3700 | about -3.7 V |
| 20:1 (S4A) | 2680 | 2200 | about -6.3 V |
| all in | 150 | 2200 (R42-44 shorted) | about -2.2 V |

Higher ratio gives a more negative anode bias, i.e. a higher threshold, which agrees with the spec table (4:1 -30 dB, 8:1 -26, 12:1 -25, 20:1 -24 dB input at threshold). All-in has the low-ish bias of the 4:1 end plus a mid sidechain level (derived; not documented in the manual).

## 5. GR control (sidechain) amplifier and rectifier

Transistors (Rev K / original-type): Q12, Q13, Q14, Q15 marked "Q12: BC107 / 2N3707"; note 5: "Q12, 13, 14 and 15 - hFE 250 or above". (Rev D numbering Q7 to Q10, 2N3707.)

| Part | Value / role |
|---|---|
| C17 | 1.0 uF / 100 V input coupling (from S9) |
| Q12 | base 4.55 V (R60 1M from collector, R61 470k to ground), collector 15.1 V with R62 47k to +30 V (27.8 V rail via R70 4.7k), emitter 4.03 V; emitter R63 4.7k to the R64/R65 junction. Phase inverter |
| Q13 | emitter follower from Q12 collector, emitter 14.54 V, R64 2.4k + R65 270 to ground (DC and AC feedback via R63 to Q12 emitter); output "phase A" |
| R68 47k | overall feedback from Q15 emitter back to the Q12 emitter node |
| R66 182k 1 %, R67 38.3k 1 % | divider from Q13 emitter to Q14 base (3.51 V). Gain/phase split |
| Q14 | collector 17.25 V via R69 44.2k 1 % (to 27.8 V), emitter R71 7.68k 1 % (to the R72/R73 divider), second phase inverter |
| Q15 | emitter follower, emitter 16.72 V, loads R72 2.4k + R73 180 to ground; output "phase B" (opposite phase) |
| C18 | 47 uF / 40 V decoupling of the 27.8 V sidechain rail (R70 4.7k from +30.3 V) |
| C19 | 6.8 uF / 35 V: Q15 emitter to CR3 anode |
| C20 | 6.8 uF / 35 V: Q13 emitter to CR4 anode |
| CR3, CR4 | FD333 (small-signal diodes, "FD333" = a 1N4148-class part) full-wave rectifiers, cathodes joined and feeding the attack network |
| R74, R75 | 47k, 47k from the two rectifier anodes down to the GRAY bias node (from the ratio A-bank); C21 100u/25 V decouples it |
| Sidechain gain | not stated in numbers; the manual says gain is high with large negative feedback; "Q12 and Q13 make up a phase inverter and emitter-follower. The output of Q13 is supplied to rectifier diode CR4 and to another phase-inverter emitter-follower combination Q14 and Q15. The output of Q15 is supplied to rectifier diode CR3. Since the two signals are out-of-phase, CR3 and CR4 full wave rectify the signals. When filtered by C27, this produces a dc voltage proportional to the signal amplitude. This is a positive-going voltage which subtracts from the bias on the VVR. To create a threshold of limiting, diodes CR3 and CR4 are biased by a dc voltage divider which is ganged with the Compression Ratio pushbutton switches." (manual 4.5; note it names C27 as the filter.) |

Signal voltages: the control-amp input node (C17/Q12 base) is 4.55 V DC. Voltage gain of Q12-Q13 and Q14-Q15 not computed (?).

## 6. Attack, release, Q-bias network (Rev K values)

Everything after the diodes:

- Rectifier cathode node -> test point YEL (19) -> **attack control**: R77 25k (1/2 W, front-panel pot, the attack pot also carries the GR-disable switch S9 in its fully-CCW "off" position) with **R76 470 ohm in parallel with C22 0.022 uF** in the same arm. This arm feeds the GR bus (wire WHT) that goes to the Q1 gate.
- **Release control**: R78 5M (1/2 W front-panel pot, "RELEASE") in series with **R79 270k** (270k across the pot's low end sets the minimum release), returning to the Q-bias node (wire BRN, 18).
- **Q-bias adjust**: R81 2k trimpot (internal), R82 10k, R83 3.9k, between the -10 V rail and 0 V (?, the exact wiring of R82/R83 to the rails is hard to follow); sets the rest voltage on the GR bus. Q1 gate rest voltage -1.5 to -2 V (hand note p-41).
- Rev D values: attack R55 25k, R54 470 with C17 .022; release R56 5M, R57 270k; Q-bias R59 2k pot, R60 3.9k, R63 10k.
- Original 1176 (Rev A, SN 101-125): attack R55 25k 1/2 W, R54 820 (?), release R56 5M, R57 270k, Q-bias R59 2k, R58 39k (?). So the network has not changed since the first version.

Time constants (derived from the schematic and checked against the spec):
- Release: R from 270k (pot at minimum) to 5.27M (5M + 270k) times the GR-bus reservoir capacitor. The only 0.22 uF part on the bus is **C27 0.22 uF** (to ground, via R7 2.2M (?) on the gate side). tau = 270k x 0.22 uF = 59 ms and 5.27M x 0.22 uF = 1.16 s. Spec: 50 ms minimum, 1.1 s maximum for 63 % recovery - consistent, so C27 is very likely the release/hold capacitor (the match is quantitative but not marked as such on the drawing).
- Attack: series R from 470 ohm (pot CW) to 470 + 25k = 25.5k (pot CCW, before the OFF switch). With C27 0.22 uF this would be 103 us to 5.6 ms, i.e. 5 to 7 times slower than the spec (20 us to 800 us). C22 0.022 uF across R76 passes the first edge (high-frequency bypass of the 470 ohm), and the real gate-side capacitance is probably smaller; the exact reservoir for attack is not identifiable on the drawing (?). The test procedure measures attack with a 20 kHz tone burst: "time duration from the peak amplitude on the leading edge of the gated signal to the smallest amplitude to be less than 20 usec with the attack pot full clockwise and greater than 800 usec when counter-clockwise (but not switched off)".
- Attack pot "OFF" (full CCW) disables limiting (S9 shorts the sidechain input to ground).
- Kick-down behaviour: the diode network means the bus only moves up when the rectified signal exceeds the diode bias; release is the slow exponential back through R78+R79 to the Q-bias voltage (voltage domain; the FET is nonlinear so the dB-domain release is not a pure exponential - derived).

## 7. Meter circuit (Rev K)

Switches S5 (GR), S6 (+8), S7 (+4), S8 (AC on) are a four-pushbutton interlocked bank; each has an A and B section. Meter M is the front-panel VU meter (SIFAM-type AL29WF in the DIY notes), lamps DS1, DS2 type #387 (Rev D: #1819), fed from the T3 secondary.
- GR mode: Q10 BF245A (original 2N5457, "URET 13-0027", selected) is a source follower from the GR bus (gate through R48 3.9M), source load R52 10k to -10 V. Source node goes through R53 15k + R54 100k trimpot ("tracking adj.") to IC1 pin 2. IC1 = 7L071 (TL071) with +30 V via R49 4.9k and C28 0.1, -10 V on pin 4, R55 2k zero trimpot (front panel, pins 3/ wiper "zero adj") with R56 10k, R57 470k and R58 100k feedback network, C16 33 pF across the amp, CR11 diode (violet wire), R59 3.6k into the meter. The needle sits at 0 VU with no gain reduction and moves downscale as the VVR bias drops ("zero gain reduction is indicated by a quiescent current through the VU meter; when limiting occurs and the VVR bias is reduced, the current in the VU meter is reduced").
- +4 and +8 mode: meter reads the output level from the X/Y secondary taps through R51 3.6k (+4) and R50 8.2k (+8). Calibration: meter reads 0 VU for +4 dBm (+8 dBm) output within +/-0.5 dB with the +4 (+8) switch (the unit's output reference is 0 VU = +4 dBm, 8 dB position = +8 dBm).
- Rev D meter driver: Q11 FET plus Q12/Q13 2N3707 differential pair, R74 2.7M, R75 2k zero pot, R71 2k front-panel tracking pot, R76 8.2k (?), R77 8.2k (?), R78 3.6k (?), R79 15k (+30 V) (numbers in Rev D are partly illegible). The text note says "Rev F: op amp for GR meter circuit".
- Calibration (manual 5.3.3, 5.3.4, 5.3.5): with no input, adjust R55 for 0 VU; apply 1 kHz -10 dB, +4 meter mode, set 0 VU with Output, switch the attack ON and adjust the input until 10 dB of limiting is indicated (-10 on the meter), then in GR mode with attack ON adjust R54 so the GR meter reads -10, with attack OFF R55 so it reads 0; repeat until both hold. Tracking error spec: within +/-1 dB. Zero drift of +/-1 dB is normal. JBL sheet p-39 gives the same sequence in 23 steps (inject 0.775 V 1 kHz; R81 CCW; input/output at "24" (mid); attack CCW, release CW, GR, 20:1; adjust output for +11 dB, R81 for +10 dB; attack CW, output 0 dB; etc.; with "+4" the internal meter reads -4 dB, with "+8" -8 dB). The "discrete meter circuit" sheet p-40 (Rev D-F): disconnect R44, centre null pot R75, voltmeter across R74, adjust R71 for GR meter zero, null pot for 0.00 V, then select R44 for tracking.

## 8. Signal line amplifier and output stage (Rev K)

- Coupling: preamp output C9 47 uF/40 V -> OUTPUT pot R27 100k, 2 W, front panel ("OUTPUT (F.P.)") -> C10 1.0 uF -> Q5 base. Q5 bias R28 10k, R29 1M, R30 200k (base 4.98 V), emitter 4.54 V with R32 2k and R33 11k + C13 100u/25 V; collector R31 68k (collector ~28.6 V); R34 4.3k feedback from the output node to the Q5 emitter; C11 47u/40 V bootstrap; C12 10 pF.
- Complementary driver: Q6 and Q7 (PNP: "2N5087 / BC560", bold "Q6: BC560, Q7: BC560"), R35 10k, R36 100, CR1 and CR2 (1N4003; bias diodes, note a 1N4003 here), R37 1k, R38 1.2k and R39 1.2k, C14 22u/25 V, C27 47 pF. Voltages: 29.7 V, 29.1 V, 28.6 V near Q6/Q7, 14.06 V Q7 collector/Q8 base.
- Output pair: Q8 (NPN: "MPS U05", bold Q8: **BD517**) and Q9 (PNP: "MPS U55", Q9: **BD518**), Darlington power transistors (BD517/518 are Darlington complements), emitter resistors R40 4.3 ohm and R41 4.3 ohm, midpoint 13.40 V; (Q8 emitter 13.47 V, 13.35 V at Q9). Class AB push-pull (the DIY notes call it class AB), supply 30.3 V single rail so the midpoint sits at about half supply (13.4 V; the schematic shows 30.3 V) (derived).
- Output coupling: C15 470 uF/40 V to **T2** primary pins 1 and 6; T2 = **Lundahl LL5402** ("163-12614", UREI part 12614). The Rev D unit used a UREI UA-5002 transformer with many windings (GRN, BLK, BLU, YEL, ORG, RED, WHT/BLK, WHT/RED, SLT, VIO) plus R35 560 and C16 .0047 Zobel (Rev D: R35 560 + C16 .0047 across the primary, R32 33 ohm (?)). In Rev K the secondary (pins 3, 4 to X and Y outputs) has R84 1k across it.
- Revision log: Rev G "output transformer loaded and optimised to operate into load or no-load; jumpers for change to European 60 ohm output impedance" (hand note); Rev F "different output amp, 16-11148 output transformer"; 1176 I "12614 output transformer, 356 IC for meter driver".
- Output: floating, transformer isolated, 150 ohm minimum load, +24 dBm into 600 ohm (12.28 V rms) maximum. Output impedance (derived from test p-33: -0.55 dB with 600 ohm, -2 dB with 150 ohm): about 39 ohm.
- Gain: spec "maximum gain 45 dB +/-1 dB" (p-05); factory test: -25 dBu 2 kHz in (both pots clockwise, no load) gives +22 dBu +/-1 dB out, i.e. 47 dB; the note says units with FET installed and biased, limiting off, have about 1 dB less gain. Hand note: "gain -25 in, +20 out (+/-2 dB), was +21.5".

## 9. Power supply (Rev K)

- T3 = UREI 16-11754, mains transformer, line selector 115 V / 230 V, fuse F1 1/4 A for 110-120 V and 1/8 A for 220-240 V (the factory test says 250 V 0.25 A 3AG slow blow), IEC inlet P1. Transformer drawing (p-58): primary 1200 turns #34, secondary 350 turns #30 centre-tapped, wound bifilar; output must be at least 35 V DC at 150 ohm and not exceed 48 V open circuit; heat rise at 160 mA with 130 V 50 Hz line must not exceed 40 C.
- Secondary windings (RED, RED/YEL, RED): four 1N4003 (CR7, CR8, CR9, CR10) make an unregulated positive supply (about +35.4 V at the 7824 input; the 1990 factory test sheet gives 40.5 V +/-1.5 V at 120 V line at the VR1 input, which disagrees with the 35.4 V printed on the drawing; the drawing's static voltages were added 2023 and may be a measurement at another line voltage) and a negative one; C25 1000 uF/50 V, C26 1000 uF/35 V, C24 1000 uF/35 V, C23 6.8 uF/35 V.
- +30 V: VR1 = 7824 (24 V three-terminal regulator) lifted by R87 1100 ohm (1 W) and R89 220 ohm: R87 from regulator output to the common pin, R89 from the common pin to 0 V, giving +30 V (30.3 V on the drawing; test limits +30 V +/-0.5 V at the collector tab of Q8 (older text) or +/-1 V at VR1 output). Output "+30 V at 50 mA, ripple 0.15 mV (?) with load". Line regulation test: difference between 100 V and 140 V line must be less than 500 mV.
- -10 V: CR6 = 10 V zener (1N4740A) with R85 1k and R86 1k, "-10 V ripple <0.15 mV with load" (?). Test: anode of CR6 -10.0 V +/-0.5 V.
- Rev D supply: zener-regulated +30 V (CR8 30 V 1 W (1N5368A-type)), R80 75 ohm 3 W, C24/C25 2000 uF/50 V, CR9 10 V 1N4740, R61/R62 1k; transformer PT-983; fuse 3AG 1/4 A slo-blo; lamps #1819.
- Power consumption: under 10 W (120 V, no signal).

## 10. Specifications (manual p-05, p-06)

| Item | Value |
|---|---|
| Input | balanced bridging, differential amplifier; 20 kohm balanced, 10 kohm single-ended |
| Max input level | +20 dB (ref 0.775 V rms) = +20 dBu |
| Max gain | 45 dB +/-1 dB |
| Frequency response | +/-1 dB, 20 to 20,000 Hz (factory test: +0.5 / -1 dB; -3 dB at about 55 kHz +/-10 %) |
| Output | floating, transformer-isolated, load 150 ohm or more, max +24 dBm into 600 ohm (12.28 V) |
| Distortion | less than 0.5 % THD, 50 Hz to 15 kHz with limiting, at 1.1 s release (distortion at low frequencies increases with faster release) |
| S/N | greater than 81 dB at threshold of limiting, 30 Hz to 15 kHz |
| Attack | less than 20 us for 100 % recovery; adjustable to 800 us |
| Release | 50 ms minimum, 1.1 s maximum (for 63 % recovery) |
| Ratio | 4:1, 8:1, 12:1, 20:1 (interlocking pushbuttons; all-in is the extra mode) |
| Threshold (min limiting, +/-2 dB input level, relative output with output gain set to give about 10 dB reserve) | 20:1 -24 dB / +10 dBm; 12:1 -25 dB / +9 dBm; 8:1 -26 dB / +8 dBm; 4:1 -30 dB / +7 dBm |
| Power | 100-125 or 200-250 V AC, 50/60 Hz, less than 10 W |
| Size / weight | 483 x 89 mm (19 x 3.5 in), depth 203 mm behind panel, 5 kg |
| Controls | Input, Output, Attack (20-800 us; CCW = switch to OFF), Release (50 ms to 1.1 s), four ratio buttons, three meter-function buttons (GR, +4, +8) plus the AC power switch (when any meter button is depressed the unit is on; the fourth button switches AC off), GR "0" adjust trimmer through a hole between Input and Output |
| Stereo link | rear phono jack; 1176SA adapter couples two units |

Transfer curve (Fig. 3-1, read off a picture, approximate): output vs input with 4:1, 8:1, 12:1 and 20:1 curves. Input axis -35 to -10 dB (about); output axis -10 to +10 dB. Unity line has slope 1. Thresholds (knee start) at about -30 dB input (4:1), -27 (8:1), -26 (12:1), -25.5 (20:1) with output about -2.5, +0.5 (?), +2 and +3 dB; the 4:1 curve has a much softer, rounded knee; the curves end at -10 dB input at about +3.4 (4:1), +3.5 (8:1), +3.6 (12:1) and +4.3 (20:1) dB output (approximate, (?)). Text: "ratio of 8 dB input increase giving 2 dB output increase is 4:1; up to 8:1 compressors, more than 8:1 limiters".

## 11. Factory test numbers worth modelling (p-31 to p-38)

- Ratio test (p-35): at 2 kHz, 1 dB limiting, 20 dB more input raises the output by 1 dB (20:1), 1.66 dB (12:1), 2.5 dB (8:1) and 5 dB (4:1), each within 20 %; the 4:1 setting needs 3 dB of limiting for the reference because of "the soft knee in the threshold circuit for this ratio". Same at 20 Hz and 20 kHz.
- Ratio switching envelope (p-37): switching from 20:1 to 12:1, 8:1, 4:1 at the same input, the output level drops by 1, 1.5, 2.5 dB (the threshold sensitivity differs); raising the input 20 dB, the maximum difference in output between ratios is less than 1.5 dB.
- Threshold (p-37): 2 kHz -20 dBu in, input at max, find the input where the meter shows only 1 dB of GR: -24 dBu +/-2 dB at 20:1, -25 dBu at 12:1, -26 dBu at 8:1, -30 dBu at 4:1; the same at 20 Hz and 20 kHz.
- Attack/release (p-36): conditions 2 kHz, -10 dBu, 20:1, GR meter, 10 dB limiting; tone burst 1 ms on / 1.5 s off with 20 kHz carrier; attack under 20 us (pot CW), over 800 us (pot CCW); release measured as 63 % recovery after switching the input down to -30 dBu: no less than 50 ms (pot CW) and no more than 1.1 s (pot CCW).
- THD (p-37): 30 Hz, 200 Hz, 2 kHz, 15 kHz at 1 dB below clip: under 0.3 % with Q1 removed; with Q1 biased correctly under 0.3 % with limiting switched off; spec "50 Hz to 15 kHz in limiting at any level up to rated output under 0.5 %". Suggested real-world check: 5 dB of limiting.
- Noise (p-35): 30 Hz to 15.7 kHz band (6 dB/oct slopes), 600 ohm termination, input and output pots CW: output noise below -57 dBu; with the input pot CCW below -63 dBu. S/N at threshold: 74 dB at 4:1 (threshold -30 dBu), 80 dB at 20:1 (-24 dBu). Hand notes: noise -60 dB (in at CW), -55 dB (?) (was -61 dB).
- Maximum levels (p-33): 150 ohm load, output clips above +20 dBu; with the input pot set 40 dB down, input clips above +20 dBu (7.75 Vrms).
- Frequency response (p-33): +24 dBu out, 20 Hz to 20 kHz: no more than +0.5 dB / -1 dB; -3 dB at ~55 kHz +/-10 %.
- Output impedance (p-33): 600 ohm load drops 0.55 dB +/-0.1; 150 ohm drops 2 dB +/-0.1 (about 39 ohm). 2 kHz square wave: no ringing, with no load or 600 ohm.
- Bias (p-34): R81 fully CCW, attack off, release CW, 20:1: at +10 dBu output set R81 for exactly 1.0 dB drop (note: Q1 must be removed to check there is no conduction without it).

## 12. Operating notes from the manual (p-14 to p-19)

- "Compression ratio" defined as delta-dB input / delta-dB output; up to 8:1 compressor, above 8:1 limiter.
- Suggested starting settings: Input CCW, Output half, Attack fully CCW then switch ON, Release half, ratio 8:1, meter +4.
- Initial check: 0 dBu in, Input about half rotation, Output set so the VU reads 0; the unit's output reads about +4 dBm; with the +8 button the meter drops 4 dB.
- Lowest threshold -30 dB in 4:1 ("if the signal applied to the input is about -20 dB, then the 1176LN will provide a maximum of 10 dB compression with the Input Level control set to maximum").
- Attack: fast transients are limited when the attack is fast; slow attack passes initial peaks (percussive sounds); release: too short pumps/breathes and distorts at low frequency, too long lowers soft passages; half-rotation is a good starting point for both.
- Stereo link (3.6): two units interconnected through the 1176SA: ratio and attack/release controls then interact; release time on one unit set to maximum, the other controls release; attack controls interact; a pair should track within at least 10 dB of GR.
- The unit has no program-dependent (auto) release; the release is the plain manual RC. (The manual does not mention a program-dependent or fast/slow two-stage release for the 1176LN.)

## 13. Revision history (p-45 to p-47, hand-written)

- Rev A 101-216: original, FET input amplifier, GR FET gate coupled to the input. Rev B 217-1078: two-transistor input amplifier; power transformer PT5101 or 4A861A; revised FET coupling to the feedback of the preamp. Rev C 1079-1238 (1176LN black front panel): L-N module on the 1176B board, bootstrap connection for lower third-harmonic distortion, 3.5 dB lower gain, 5 dB lower GR sensitivity, 5 dB lower noise. Rev D 1239-2331: L-N modification on the PC board, changed to 13-0027 FET, power transformer PT-983, grounded power cord, meter lights. Rev E 2332-2610: 10M resistor on the ratio switch, 11754 power transformer, 115/230 switch. Rev F 2611-7052: op amp for the GR meter circuit, 16-11148 output transformer, different output amp. Rev G 7053-7651: bridging (differential IC) input, threshold lowered by 6 dB, lower noise with input control below 3/4, IC voltage regulator in power supply, output transformer loaded and optimised to operate with or without load, jumpers for European 60 ohm output impedance. Rev H 7652 onward: new look front panel, back-lit VU meter, IEC power connector, meter terminals removed. Rev I: 12614 output transformer, 356 IC for the meter driver, no modification needed for export output impedance. J, K listed on the log (no text).

## 14. Voltages marked on the Rev K drawing (static DC, for bias reference)

Q1/Q2 node 1.02 V; Q2 collector 1.76 V, emitter 0.51 V; Q3 collector (Q4 base) 11.98 V; preamp supply 25.8 V (after R17 6.8k); Q4 emitter (preamp output) 11.5 V; Q5 base 4.98 V, emitter 4.54 V; Q6/Q7 node 28.6 / 29.1 / 29.7 V; Q7 collector 14.06 V; Q8 emitter 13.47 V, output midpoint 13.40 V, Q9 emitter 13.35 V, Q9 base 12.77 V; supply 30.3 V; control amp Q12 base 4.55 V, emitter 4.03 V, collector 15.1 V; Q13 emitter 14.54 V; Q14 base 3.51 V, collector 17.25 V; Q15 emitter 16.72 V; sidechain rail 27.8 V; unregulated 35.4 V.
