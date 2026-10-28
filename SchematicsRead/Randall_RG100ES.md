# Randall RG100ES / RG80ES: schematic and owner's manual, read stage by stage

Sources: `randall-1.png` (the schematic, one rotated page, 150 dpi scan, mostly legible after rotating and enlarging), `randall-2.png` (manual cover), `randall-3.png` (front panel text), `randall-4.png` (back panel text), `randall-5.png` (block diagram and specifications).
Drawing: Randall Instruments Inc., Irvine CA. Title block lists the schematic as covering RG80-112SC, RG80PH, RG80ES, RG100ES and RG100HT; part number PCB 21 46B, "Model # LEFT", revision A "UPDATE" dated 9-85 (read). Parts only used on the RG100HT (balanced XLR / RCA / signal-level pot with a 1k, 100 ohm and 2k pot arrangement) are marked "USED ON RG100HT ONLY".
Notes printed on the sheet: all capacitors in MFD at 100 V; all resistors 1/2 W unless noted; IC is a 4558; a circled number is the DC voltage from that point to chassis ground (all voltages +/-10 %); a boxed number is signal at 1 kHz; Q1 to Q7 are TIS-58 FET or 2N5484 (N-channel JFET); Q13 to Q16 are 2N6254; Q10/Q12 are TIP32C and Q11 is TIP31C.

Everything marked (computed) is my arithmetic from the printed values; everything marked (assumed) uses a device figure that is NOT on the sheet. Everything marked (?) is not legible.

## 1. What the amp is (manual, page 5 block diagram)

All-solid-state head, two channels, one common pair of inputs (Hi +6 dB and Lo 0 dB), each channel its own FET preamp (FET preamp, then FET gain stage with the gain pot between, then a FET follower, then a master), the masters feeding one four-control tone block (Treble, Middle, Bass; Presence is after it), a FET follower, then an effects send/return, then the power amp. Spring reverb is a side loop (drive op-amp, tank, return op-amp, reverb pot) that adds back into the effects bus. Channel select is a footswitch with a red and a green LED.

Channel naming: the manual's "#1" is the upper path in the block diagram (the **green** channel on the schematic, Q1 to Q3); "#2" is the **red** channel (Q5 to Q7), which is the one with the extra clipping parts (see section 4) and the one the Treble pull-switch acts on ("acts somewhat like a compressor to the #2 channel only").

Specification table (manual page 5), verbatim values:
- Power: RG100ES 120 W RMS at 4 ohm and 80 W RMS at 8 ohm, 2 % THD. RG80ES 100 W RMS at 4 ohm and 80 W RMS at 8 ohm.
- Minimum input for rated output: 0.5 mV at 3 kHz (#2 channel), 6.5 mV at 3 kHz (#1 channel). That is 22.3 dB (computed) more gain in the red channel than the green.
- Tone controls all at 5: Bass swing 15 dB at 50 Hz; Mid swing 10 dB at 600 Hz; Treble swing 16 dB at "K Hz" (the number is missing in the scan (?), 10 kHz would fit the nodal solve below); Presence swing 15 dB at 10 kHz and 18 dB at 15 kHz.
- Noise: all tone controls at 10, volume 0, master 10: -75 dB.
- Effects loop: send 200 mV max, return 50k. The manual says the loop is "prepower amplifier and post pre amplifier".
- Signal output 0 dB into 600 ohm. Input impedance: 1 meg (Hi +6 dB), 130k (Lo 0 dB).
- **Power amplifier damping factor 2 to 4 at 8 ohm.** (This matters, see section 9 and the comparison file.)
- Weight 51 lb. RG100ES 11 in high, 29 1/4 wide; RG80ES 9 in, 24 in.
- Back panel: speaker jacks "100 WATTS RMS 4 ohm, 80 WATTS RMS 8 ohm" (two jacks, minimum load 8 ohm "less than 8 OHM not recommended"; the page says 4 ohm rating on the panel and 8 ohm in the text: contradictory, as printed), pre-amp output "0 dB 600 ohm", effects send/return, reverb footswitch jack, channel footswitch jack, unswitched 200 W AC outlet, 2.5 A circuit breaker. Line draws about 200 W at full output.
- Front panel (page 3): On/Off three-position switch that also reverses mains polarity ("select the on position which best grounds the amplifier"), pilot lamp, Reverb, Presence, Bass, Middle, Treble, Master 2 / Gain 2, Master 1 / Gain 1, two input jacks. The Treble knob pulls out for the "Sustain Boost" ("adds clipping-diodes to the circuit"; overall volume drops; "infinite sustain"; for studio or home practice; "only a portion of the rated power output can be obtained when operated in this manner"). Manual's advice for distortion: channel gain at 10 and set the level with the master; for clean: master at 10 and the channel gain sets the level.

## 2. Supply (schematic lower left)

- Mains: three-position on/off/reverse switch with pilot lamp, 2.5 A breaker, C37 0.005 uF across the line, unswitched outlet.
- Transformer secondary label: "2-24 RG100ES + RG100HT / 2-12 RG80-112 + RG80ES" (read as two 24 V windings for the 100 W models and two 12 V for the 12-inch models; unclear (?)). Bridge marked "25 AMP 200 VOLTS". With a 24-0-24 secondary the unloaded rails would be about +/-32 V (computed: 24 x 1.414 minus two diode drops), not the +40 V printed on the tabs, so the label probably means something other than a 24-0-24 winding (?).
- Power rails: C38 6000 uF / 50 V and C39 6000 uF / 50 V (one per rail), tabs "+40 V DC" and "-40 V DC".
- Preamp rail: R6 1k / 1 W from +40 V into a 24 V zener with C45 2200 uF / 35 V, tab "+24 V DC".
- Op-amp rails: R58 and R59 1.5k / 1 W from the +40 / -40 rails into 12 V zeners D9 and D10 with C40, C41 0.1 uF and C42, C43 50 uF / 50 V, tabs "+12 V DC" and "-12 V DC" (the 4558 pins 8 and 4).
- DC-point annotations in the FET stages imply a rail of about +28 V, not 24 V (computed): the gate divider R13 1M / R14 1M is annotated +14 V, and the Q1 drain (33k) at +15 V with a +1 V source (2.2k) needs about 0.39 mA, so 15 V + 0.39 mA x 33k = 28 V. Both disagree with the "24 V ZENER" label by 17 % (more than the stated 10 % tolerance). Not resolved.
- Ripple at full power (computed): 80 W into 8 ohm needs 35.8 V peak, 4.47 A peak, mean rail current 1.42 A, so ripple is about 2.0 V peak-to-peak at 120 Hz on 6000 uF (5 % of 40 V); 120 W into 4 ohm needs 31 V peak, 7.7 A peak, 3.4 V peak-to-peak. Transformer regulation (sag) is not given on the sheet, so DC droop under load cannot be computed.

## 3. Input and green channel (Q1 to Q3)

Inputs: HI jack through R1 68k, LO jack through R2 68k, joined at one node that goes to both channels. The Hi jack's switch contact grounds its side when nothing is plugged in (drawn), so in the Lo jack the source sees R2 68k in series and R1 68k to ground: -6 dB (computed), and the Lo input impedance is 68k + 68k = 136k, which matches the spec's "130K ohm Lo". The Hi input impedance reads 500k here (R3 1M and R20 1M in parallel, computed) against the spec's "1 MEG": a small discrepancy.
Annotated signal at the Hi jack: 6 mV.

Green channel components:
- C1 0.01 uF to R3 1M to ground: high-pass 15.9 Hz. C2 250 pF to ground with the 68k source: low-pass 9.4 kHz.
- **Q1**: JFET common-source. Drain load R4 33k to the rail, source R5 2.2k to ground **with no bypass capacitor**. Annotations: drain +15 V, source "+1 V" (printed with a stray minus sign, read as +1 V). Annotated output 40 mV at the gain pot top. Stage gain (annotation based): 40 mV / (6 mV x 0.88 input divider) = 7.5 (17.5 dB) (computed). My estimate with an assumed gm of 1.1 mS and the 50k pot as load: -gm.Rd/(1+gm.Rs) = -6.4 (assumed gm, agrees).
- C4 0.02 uF coupling into the gain pot, 50KA (audio taper), 159 Hz corner with the pot's 50k (96 Hz including the 33k drain). C5 0.005 uF across the upper part of the pot (bright / treble bleed: from the pot top to the wiper node), R7 1k in series from the wiper to the Q2 gate.
- **Q2**: JFET common-source, drain load R8 33k, source R9 2.2k **bypassed by C7 10 (uF)**, so full gain; C6 20 pF from drain to gate (Miller roll-off: effective input capacitance about 20 pF x (1 + gain) = about 1.1 nF at a gain of 55, with R7 1k plus the pot's source resistance; at the pot at its middle (12.5k) that is about 10 kHz; at the pot's top it is lower). Annotations: source +1.3 V ("+L3" read as +1.3), drain +11.5 V (the Q3 gate), and annotated 2.1 V at the master input, so the Q2+Q3 gain from the 40 mV node is 2.1 V / 40 mV = 52 (34.4 dB) (computed). That implies a gm near 1.7 mS (assumed Q3 gain 0.93, no loading).
- **Q3**: JFET source follower, source R10 4.7k, gate +11.5 V, source +12 V, drain on the rail. C8 1 MFD (1 uF, polarised) into the **Master** 10k pot. Annotation "2.1 V" at the master (at 1 kHz, the nominal level for rated output).
- The green channel has **no clipper**: no diodes or zeners in this channel. It stays clean until the power amp runs out of swing.
- LED indicator D2 "GREEN" LED with a 4 V zener and R19 1.5k / 1 W; D1 "RED" LED with a 4 V zener and R18 1.5k / 1 W; both from the footswitch socket on the chassis rear (tab symbol; which rail they hang from is not clear (?)).
- Overall green chain at the printed annotations: 6 mV (jack) to 2.1 V (master input) = 350 (50.9 dB, computed). Then master (0 dB at full) then R11 10k into the tone stack.

## 4. Red channel (Q5 to Q7), the clipping channel

- C15 0.01 uF to Q5 gate (R20 1M to ground, C16 250 pF to ground): 15.9 Hz and 9.4 kHz as in the green channel.
- **Q5**: JFET, drain R21 33k, source R22 2.2k **bypassed by C17 0.47 uF**. The bypass gives the stage a low-frequency shelf: below the zero at 1/(2 pi x 2.2k x 0.47 uF) = **154 Hz** the source is unbypassed (gain about gm.Rd / (1 + gm.Rs)), above 154 x (1 + gm.Rs) = about 520 Hz it is bypassed (gain about gm.Rd). The step is **20 log (1 + gm.Rs) = 10.7 dB for an assumed gm of 1.1 mS (13.4 dB at 1.7 mS)** (computed, gm assumed, not on the sheet): a bass-shy first stage that is a shelf of about 11 to 13 dB between 150 and 520 Hz, not a high-pass. (This is the red / lead channel. The *unbypassed* first stage with no tilt at all is the green channel's.)
- C18 0.02 uF into R23 22k in series, then the gain pot 50KA; C19 0.005 uF across R23 / to the wiper (bright). R24 1k from the wiper to the Q6 gate. Because R23 22k sits ahead of the pot, the first stage always sees at least 22k + (pot to ground), and the pot's attenuation law includes that 22k.
- **Q6**: JFET, drain R25 33k, source R26 2.2k bypassed by C21 10 (uF); C20 20 pF drain-to-gate (Miller roll-off as in Q2). **Q7**: source follower, source R27 4.7k.
- **C22 1 MFD** then the clipper (below), then the Master 10k with a **0.002 uF** (0.002 uF across the top of the master to the wiper: a treble bleed that only the red master has).
- **Clipper D3 / D4: two zeners labelled "4 V Z", anode to anode** (D3 cathode to the signal node, D4 cathode to ground, read from a 10x enlargement). Each side clips at one zener (4 V, if the label is the breakdown) plus one forward drop of about 0.6 V: **about +/-4.6 V** (computed). The existing notes in CIRCUITS.md read "4.7 V zener pairs, about +/-5 V"; the sheet says 4 V. Placement: the clipper is on the **master's input** node, after all gain and before the master; so the master never changes the clipping, only the level after it (consistent with the manual: "channel gain at 10 and the master sets the volume").
  Level reference: the nominal 2.1 V at that node, so the clip sits 6.8 dB above the nominal drive level (computed 20 log(4.6 / 2.1)). The tone stack then takes the level down (section 5).
- **Sustain Boost (Treble knob pulled out)**: D5 and D6, two 1N914 in anti-parallel, from that same pre-master node to ground through the pull switch. With the switch closed the node is clamped at about +/-0.6 V (computed), 17.7 dB below the zener clip (computed 20 log(4.6 / 0.6)), at a very soft knee: this is the "volume drops, infinite sustain, compressor-like" mode of the manual.
- Red channel gain: from the spec, 0.5 mV for rated output against 6.5 mV in the green, so 22.3 dB more gain. From my assumed gm: Q5 (about 34) x Q6 (about 56) x Q7 (0.93) x input divider (0.88) = about 1560 (63.9 dB, assumed gm), against 4200 (72.5 dB) implied by 2.1 V / 0.5 mV: the 8.6 dB gap is not explained (the spec figure is probably quoted into clipping, or gm is higher).

## 5. Tone stack and Presence (all legible; nodal solve done)

Both masters feed the same node through R11 10k (green) and R28 10k (red). Topology as read (my reading of the wiring; nodes: IN = after R11/R28, L, T, B, wiper W = output):
- IN to C9 0.002 uF to the top of the **Treble** pot (50K, taper not marked "A" so read as linear); the treble pot's **wiper is the stack output**.
- IN to R12 4.7k to node L. L to C10 0.22 uF to node T (the treble pot's bottom end and the **Bass** pot top).
- Bass pot 50KA used as a rheostat (wiper tied to its lower end) from T to node B; B is the top of the **Mid** pot (2K, linear). Mid pot bottom to ground. The mid pot's wiper goes through **C11 0.2 uF** to L.
- Output W goes through C12 0.005 uF to the Q4 gate (R13 1M / R14 1M, 500k in parallel) and, at the same node, to the reverb send op-amp through C23 0.005 uF.
- Source resistance of the stack: R11 or R28 10k plus the master pot's output impedance.

Corner frequencies (computed): treble C9 with 50k: 1.59 kHz; bass C10 with the 4.7k: 154 Hz and with 50k: 14.5 Hz; mid C11 with 2k: 398 Hz.

Nodal solution (computed, source 10k, load 500k and 5 nF, bass pot audio taper with 10 % at mid rotation (assumed), treble and mid linear):
- Bass swing: 16.0 dB at 50 Hz (spec 15 dB). Mid swing: 9.1 dB at 600 Hz (spec 10 dB). Treble swing: 12.6 dB at 3 kHz, 13.7 at 5 kHz, **14.3 dB at 10 kHz** (spec 16 dB at "K Hz"). All three within 1 to 2 dB of the manual, which checks the topology reading.
- Insertion loss, all controls at 5: -15 dB at 100 to 200 Hz, -19 dB near 600 to 1000 Hz, -14 dB above 4 kHz (a scoop of about 4 to 5 dB at noon, centred near 800 Hz). With all three at 10 it is -13.9 dB at 1 kHz, against the **printed 2.1 V in, 400 mV out = -14.4 dB** (computed): a second check (the annotation was evidently taken with the tone controls at maximum).
- Mid control alone: at 0 the notch is 8 to 9 dB deep at 757 Hz (-25.9 dB against -17.5 at 4 kHz); at 10 it is flat to a slight boost; the control sweeps the scoop from deep to none.
- Table (dB re input, source 10k):
  | setting (B / M / T) | 50 | 100 | 200 | 400 | 800 | 1.6k | 3.2k | 6.4k | 12.8k |
  |---|---|---|---|---|---|---|---|---|---|
  | 5 / 5 / 5 | -17.9 | -15.2 | -15.2 | -17.6 | -19.2 | -16.8 | -14.9 | -14.2 | -14.0 |
  | 10 / 10 / 10 | -9.1 | -9.7 | -13.4 | -17.0 | -15.2 | -11.5 | -9.4 | -8.6 | -8.4 |
  | 0 / 0 / 0 | -28.4 | -25.0 | -24.0 | -24.3 | -26.2 | -29.9 | -34.9 | -40.7 | -46.6 |
  | 7 / 2 / 8 (metal) | -13.7 | -12.2 | -14.6 | -20.6 | -21.2 | -15.6 | -13.0 | -12.1 | -11.9 |
- The stack output is the Q4 gate (400 mV annotated).

**Q4** JFET source follower: gate bias R13 1M to the rail and R14 1M to ground (gate annotated +14 V), source R15 10k (+15 V), drain on the rail; C13 10 uF (polarised) from the source into R16 1k and R17 2.2k to the effects node. C12 0.005 uF into 500k: high-pass 63.7 Hz.
**Presence**: a 2K pot with its wiper tied to its lower end (rheostat) in series with C14 0.1 uF from the R16/R17 node to ground. So it is a shunt from the node to ground through (Rp + C14), driven from R16 1k: H = (1 + s Rp C14) / (1 + s (R16 + Rp) C14). With Rp = 0 it is a first-order low-pass at 1.59 kHz (-16 dB at 10 kHz, computed); with Rp = 2k it is a shelf (zero 796 Hz, pole 531 Hz) that ends at -3.5 dB. So the Presence control is a *treble cut that is removed as the knob goes up*, spanning about 12.5 dB at 10 kHz (computed; the manual says 15 dB at 10 kHz and 18 dB at 15 kHz, so the real Q4 output impedance and the following R17 2.2k add a little more).
Annotated level at the effects send node: 250 mV (so Q4 + Presence network lose about 4.1 dB from the 400 mV annotated at its gate).

## 6. Effects loop

Send and return jacks sit in the bus after R17 2.2k (send 200 mV max, return 50k per the spec). The reverb return is injected at R38 47k into the same bus (via the reverb pot). The jack contacts are drawn with a switch that normals the bus when nothing is plugged in.

## 7. Reverb (spring, "Reverb Type IV" tank)

- Send tap: the tone-stack output node through C23 0.005 uF into a **4558 half (pin 3, R31 1M to ground)**: a non-inverting stage with R32 470k feedback, R29 47k to ground, and R30 4.7k with C24 0.022 uF in series from the inverting input to ground. Gain (computed): **11 (20.8 dB) at low frequencies, rising to 111 (40.9 dB) above 1.54 kHz**, with the transition starting near 154 Hz. So this stage is a strong treble pre-emphasis (+20 dB) for the spring.
- Output: C25 3.3 (uF) and R33 100 ohm to the tank input coil. The tank's winding impedances are not printed (?).
- Return: tank output through C26 0.01 uF into the second 4558 half (R34 1M to ground at the input). Non-inverting with feedback R35 1M in parallel with (C28 50 pF in series with R36 470k), and a ground leg of R37 10k in series with C27 0.01 uF. Gain (computed): 1 at DC, **rising 20 dB per decade from about 16 Hz (1/(2 pi x R35 x C27)) to 101 (40 dB) at 1.59 kHz (1/(2 pi x R37 x C27))**, flat above that; the C28 + R36 branch lowers the gain again above about 6.8 kHz (1/(2 pi x R36 x C28)).
- C29 0.22 uF to the 2K reverb pot (the wiper through R38 47k into the bus). The reverb footswitch jack shorts the return output to ground when off.
- Net: send +20 dB tilt (154 Hz to 1.54 kHz), return +40 dB tilt (16 Hz to 1.59 kHz); total spring loop tilt of about 60 dB between 16 Hz and 2 kHz, with a roll-off above about 7 kHz. (The plugin already has a "Tilt" reverb style from this sheet; see the comparison file.)

## 8. Power amplifier (all read; wiring of two details uncertain)

Topology: NPN differential input pair **Q8 / Q9 (TIS98)**, tail R42 10k to a node at -27 V (R43 4.7k to -40 V, C31 4.7 uF / 35 V decoupling, emitters at -1 V). Q8's base is the input (C30 0.22 uF, R39 1k, R40 47k to ground, C3 500 pF to ground: high-pass 15 Hz, low-pass about 318 kHz); Q9's base is the feedback node. Q8 collector load **R41 680 ohm** to +40 V (collector annotated 38 V, so about 2.9 mA in R41), Q9's collector straight to +40 V.
Voltage amplifier: **Q10 (TIP32C, PNP)** with R47 22 ohm in its emitter to +40 V, **C32 500 pF** from its base to its collector (Miller compensation), collector (annotated +1.2 V) into a bias spreader of **D7 and D8 1N914 in series** (D7 marked "TRM", thermal sensing), a 22-ohm (?) resistor across D8, and a **250 ohm trim pot** with R48 1k and R49 1.5k / 1 W below it (the R48/R49 node at -15 V), R51 100 ohm and R50 10k, C44 0.001 (?). **C35 50 uF / 50 V** bootstraps from the output node to the R48/R49 node.
Output stage: quasi-complementary. Upper: Q11 (TIP31C driver, R53 100 ohm) feeding the NPN pair Q13 and Q15 (2N6254) with emitter resistors **R55 0.6 ohm and R57 0.27 ohm, 7 W each**. Lower: Q12 (TIP32C PNP driver, R54 100 ohm) feeding the NPN pair Q14 and Q16 (2N6254) with **R56 0.6 ohm and R60 0.27 ohm, 7 W**; the lower pair is the sinking side to the -40 V rail (Sziklai-type: PNP driver into NPN outputs). Two 1N4003 diodes from the output node to the rails. The speaker return goes through **R63 0.18 ohm / 10 W** to chassis ground. R62 2.2k to the signal-output jack (and one more unlabelled resistor in that path (?)).
Annotated: 20 V at the 8 ohm speaker jack when 250 mV is at the effects node: gain 80 (38.1 dB, computed). Rated 80 W / 8 ohm is 25.3 V rms, so the 20 V is 50 W.

**Feedback.** The base of Q9 is fed by (a) the output node through R46 22k and R45 22k (44k), with R44 330 ohm and C33 10 uF / 16 V to ground, and (b) the speaker-return node (the top of R63) through R52 1.5k and C36 10 uF. C34 10 uF is also drawn from the R45/R46 junction to ground (read as drawn; a 0.7 Hz low-pass with R46 would make path (a) DC-only, which does not agree with the 80x gain annotation, so I treat C34's connection as unresolved (?)).
Computed with both paths active (voltage path through 44k, current-sense path through 1.5k, R63 0.18 ohm, R44 330 ohm to ground):
- Gain at 1 kHz: 163 (44.3 dB) open circuit, 123 (41.8 dB) into 16 ohm, **99 (39.9 dB) into 8 ohm**, 72 (37.2 dB) into 4 ohm. The annotated 80 (38.1 dB) is 1.8 dB lower (within the stated tolerances).
- **Output impedance 5.0 to 5.2 ohm** (computed from the open-circuit and loaded gains), i.e. a damping factor of **1.5 at 8 ohm** (computed); the manual says "2 to 4 @ 8 ohm". The direction is certain: this power amp has a source resistance of several ohms, because the 0.18 ohm sense resistor feeds a current-proportional term into the feedback. It is nothing like the stiff (<0.1 ohm) output of a normal solid-state amp. The 8 ohm to 4 ohm gain difference is -2.7 dB (computed).
- Bass shaping from C33, C36 (10 uF each): response relative to 1 kHz at 8 ohm (computed): -0.8 dB at 100 Hz, -2.4 dB at 50 Hz, -4.5 dB at 30 Hz, -6.3 dB at 20 Hz, -9.3 dB at 10 Hz. Together with the input high-pass (15 Hz) the amp rolls off below about 40 Hz.
- Rails +/-40 V, clipping hard and symmetrically near +/-37 V at the output (the output stage and the 0.27 / 0.6 ohm emitter resistors cost a couple of volts). Because the pre-master clip level is 4.6 V and the stack takes 14 dB off, a hard-clipped red channel at full master presents about 0.9 V (computed 4.6 V x 0.19) times a power-amp gain of 80 to 100 = about 70 to 90 V against about 36 V available: **at full master the power amp is itself driven 6 to 8 dB into hard clipping** after the zeners have already clipped. At -8 dB of master it stops.

## 9. Footswitch and switching

Footswitch diagram on the sheet: a channel-select switch ("RED ON" / "GREEN ON") and a "BOTH / SINGLE" switch, wired to the footswitch socket on the chassis rear with D11 "RED" and D12 "GREEN" LEDs. The socket's wiring into the master pots (it mutes the channel not selected, by what looks like a connection from a socket pin to the green master's top) is only partly traceable (?). "BOTH" implies both channels can be live together into R11 / R28.
Reverb footswitch: shorts the reverb return output (above). Treble pull switch: D5 / D6 clamp (section 4). Input jack switching (section 3).

## 10. Controls and their values (summary)

| control | value | taper | where |
|---|---|---|---|
| Gain 1 (green) | 50KA | audio | between Q1 and Q2 |
| Gain 2 (red) | 50KA | audio | between Q5 and Q6 (with R23 22k ahead of it) |
| Master 1, Master 2 | 10k each (marked "MASTER") | not marked | after the clipper (red) / after the follower (green) |
| Treble | 50K | not marked (linear read) | tone stack, wiper = output; pull = Sustain Boost |
| Middle | 2K | not marked | tone stack, shunt network with C11 0.2 uF |
| Bass | 50KA | audio | rheostat in the stack |
| Presence | 2K | not marked | rheostat with C14 0.1 uF |
| Reverb | 2K | not marked | return level into the bus |
| Reverb footswitch, channel footswitch, mains switch | switches | | |

## 11. Level chain at the printed annotations (1 kHz, green channel, gain at maximum)

| point | annotated | step |
|---|---|---|
| Hi jack | 6 mV | |
| Q1 output / gain pot top | 40 mV | +16.5 dB (7.5 x after the input divider) |
| Q3 output / master input | 2.1 V | +34.4 dB |
| tone-stack output (all at 10) | 400 mV | -14.4 dB |
| effects node (after Q4 and presence) | 250 mV | -4.1 dB |
| 8 ohm speaker output | 20 V | +38.1 dB |
| overall | 6 mV to 20 V | 70.5 dB (3333 x) |

## 12. What could not be read

- Treble swing frequency in the specification ("K Hz").
- Transformer secondary voltages and which "2-24" means (see section 2).
- Q10 collector bias network details: the 22 ohm (?) across D8, a "47 ohm" label near R51 (?), the exact value of C44 (read as 0.001 (?)).
- Whether C34 is genuinely in the feedback network (section 8).
- Exact footswitch wiring and which rail feeds the channel LEDs (R18, R19).
- An unlabelled resistor next to R62 in the signal-output path.
- The unlabelled value of the D3/D4 zeners beyond "4 V Z".
- All DC levels not circled; the output transformer-free design has none.
