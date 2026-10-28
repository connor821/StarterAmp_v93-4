# Roland JC-120 / JC-160 service notes: schematic read (8 pages)

Source: `reference/schematics/Roland_JC-120_JC-160_service_notes_5th_ed_1983.pdf` (pages 1-4) and `..._3rd_ed_1979.pdf` (pages 5-8), rendered at 130 dpi (1190 x 840 pt). Both service-note sets were read from the page PNGs; dense areas were cropped and enlarged 3 to 6 times. Nothing in the code or the existing docs was changed.

How to read the confidence marks: a value written plainly was legible. `(?)` = not legible or ambiguous. "Derived" = my arithmetic from legible values (the Python solves are described at the end). "Assumed" = a number that is not on the page.

## 0. What each page is

| Page | Edition | Content |
|---|---|---|
| 1 | 1983 (5th ed., 20 Jan 1983) | Title page, serial ranges, specification table, front-panel photos (JC-120 and JC-160), parts list for pots, switches, jacks, speakers |
| 2 | 1983 | PCB layouts: pre-main board CH-2 (7011629000, pcb 2291023801), pre-main board CH-1 (7011628000, pcb 2291023701), power-supply board (7011627100 / 7011627400, pcb 2291023901), fuse table, board-replacement procedure |
| 3 | 1983 | **Full schematic**: CH-1 preamp + power amp, CH-2 preamp (distortion, reverb, chorus/vibrato, BBD, LFO) + power amp, power supply, transistor/IC/diode list |
| 4 | 1983 | AC wiring drawings for 100 / 117 / 220 / 240 V, parts list (pots, ICs, transistors, FETs, diodes, capacitors, fuses) |
| 5 | 1979 (4th ed. header, "Mar. 20 1979 first printing, Mar. 20 1983 second printing") | Title page, 1979 specification table, photos, part numbers |
| 6 | 1979 | PCB layouts AP-114 (CH-2), AP-113 (CH-1), PS-56 (supply); **checking and adjustment table** (chorus rate, BBD clock range, vibrato LFO range) |
| 7 | 1979 | **Full schematic**: AP-113 (CH-1 + power amp), AP-114 (CH-2 + power amp, distortion, reverb, chorus/vibrato), PS-56 supply |
| 8 | 1979 | AC connection drawings, parts list |

There is **no block-diagram page** and no separate BBD board or separate reverb page. The chorus/vibrato (BBD), reverb and distortion all live on the CH-2 pre-main board (1979: AP-114; 1983: pre-main board CH-2). Channel 1 is a plain clean channel (input, tone stack, volume, power amp) with no effects.

The two editions are **different circuits** (different pot values, different tone-stack parts, a different BBD and clock chip). The 1979 sheet uses a printed-circuit pair AP-113 / AP-114; the 1983 sheet uses the pre-main boards CH-1 / CH-2. Both are covered below and labelled "1979" and "1983".

Note: `CIRCUITS.md` (b58) describes a different 1979 sheet ("AP-44", Q18 2SC1000, Q19 2SA493, Q20 2SK30A). That is not the sheet in these eight pages (these pages show AP-113 / AP-114 with 2SK117GR JFETs). The stack parts it listed (R 18k, 0.0015 uF, 0.47 uF, 0.15 uF) match what I read on AP-113 / AP-114, so it is probably an earlier edition of the same stack.

## 1. Specifications as printed

| Item | 1983 (page 1) | 1979 (page 5) |
|---|---|---|
| Output | 120 W rms (2 x 60 W) | 22 V rms per channel into 8 ohm |
| Speakers | JC-120: 30 cm x 2 (30-103D); JC-160: 25 cm x 4 (25-33D) | JC-120 30-103D; JC-120A R&P: R&P C-1230; JC-160 25-33D |
| Power consumption | 400 VA | 500 VA |
| Input sensitivity, 1 kHz | HIGH 20 mV, LOW 60 mV, MAIN IN 800 mV | HIGH 22 mV, LOW 72 mV, MAIN IN 700 mV; also HIGH 38 mV at 100 Hz and 11 mV at 10 kHz |
| Input impedance | not printed | HIGH 220 kohm, LOW 82 kohm, MAIN IN 68 kohm |
| Line out | 100 mV at 1 kHz, 100 ohm | 100 mV, 100 ohm |
| Residual noise | 7 to 35 mV per channel | 9 to 20 mV per channel |
| Tone range, VOLUME at centre | TREBLE 17 dB @ 10 kHz, MIDDLE 13 dB @ 350 Hz, BASS 14 dB @ 50 Hz, BRIGHT 5 dB @ 10 kHz | TREBLE 38 dB @ 10 kHz, MIDDLE 2 dB @ 1 kHz, BASS 18 dB @ 100 Hz, BRIGHT 4 dB @ 10 kHz |
| Weight | JC-120 28 kg, JC-160 35 kg | JC-120 28 kg, JC-120A R&P 40 kg, JC-160 35 kg |

`CIRCUITS.md` quotes "treble +/-35 or 37 dB, middle 0.6 to 0.8 dB, bass +/-17 to 18 dB". Those numbers do not match what these two pages print (38 / 2 / 18 for 1979, 17 / 13 / 14 for 1983). Section 3 shows the printed numbers are the full min-to-max range of each control with the other two at a particular setting, and my solved stack reproduces them.

Printed measurement conditions (page 3): signal voltages taken with a 1 kHz sine at the input, **BASS fully clockwise, TREBLE fully clockwise, MIDDLE fully counter-clockwise**.

## 2. Signal path, stage by stage

### 2.1 Channel 1, 1983 (page 3, top block "CH-1"). No effects.

Supply rail on this block: +30 V.

| # | Stage | Parts as read | Printed level |
|---|---|---|---|
| 1 | Inputs | HIGH jack through R2 33k; LOW jack through R1 68k into the same node (LOW is about 9.8 dB less sensitive: 59 mV vs 19 mV). C3 0.082 uF coupling, R6 1k gate stopper. D1, D2 shown dashed (input protection, not populated on the drawing?) and C1 100p to ground | HIGH 19 mV, LOW 59 mV |
| 2 | Q1 2SK117GR common-source | R5 1M gate leak to a bias node; R7 220k (+30 V) and R3 6.8k (ground) set that node (about 0.9 V, derived); R4 1k source; R9 12k drain load to +30 V; C4 47p across the stage (gate-drain, Miller/RF) | drain 180 mV, +13.6 V DC |
| 3 | Tone stack | C5 150p, R36 100k, C20 0.082, C26 **0.056** (CH-2 has 0.047), R43 1k, VR2 TRE 250kB, VR4 BASS 250kA, VR3 MID 10kB. Full topology in section 3 | not printed for CH-1; CH-2 shows 180 mV in, 24 mV out |
| 4 | Volume + Bright | VR1 VOL 1MB; BRIGHT: C6 330p + switch SLR-022 across the top half of the pot, R10 1M bleed. Section 4 | |
| 5 | Q3 2SK117GR + Q2 2SC2240GR | C8 0.082, R16 1k gate stopper, R15 1M gate leak, R14 220k / R13 4.7k bias divider (about 0.6 V, derived), C7 dashed (Miller cap, optional), R8 33k drain load, R17 2.2k source with C10 10/16 and R18 120 (source bypass: HF gain much higher than LF), Q2 (2SC2240GR) emitter follower direct-coupled from Q3's drain, R12 12k emitter | |
| 6 | Output coupling | C2 1/50, R11 47k, C9 0.082, R21 1k, Q5 2SK117GR source follower stage (R19 220k / R20 220k bias, R23 12k, R22 100k, R24 1M), C12 1/50 | **MAIN IN node 800 mV** |
| 7 | MAIN IN jack | Break-jack, C15 0.082 into the power-amp input | 800 mV = rated power |

The preamp output node and the MAIN IN jack go to power-amp input Q7 (section 8).

Derived total gain CH-1 input (19 mV HIGH) to 800 mV: 42 times (32.5 dB). Q1 gives 180/19 = 9.5 (19.5 dB). The stack is -17.5 dB at the printed measurement setting. That leaves 800 mV / 24 mV = 33 times (30.4 dB) for Q3, Q2, Q5 and the output divider. The Volume position is not stated (assumed maximum).

### 2.2 Channel 2, 1983 (page 3, "CH-2")

| # | Stage | Parts as read | Printed level |
|---|---|---|---|
| 1 | Inputs | HIGH: R4 33k; LOW: R3 68k; C1 0.082; R5 1k gate stopper; R2 6.8k and C3 100p to ground; D13 / D14 dashed; Q1 2SK117GR gate leak R1 1M; R6 220k (+26 V) bias; R8 1k source; R9 12k drain load; C2 47p | HIGH 19 mV, LOW 59 mV |
| 2 | Q1 drain | | 180 mV, +12.1 V DC (printed "+12.1", partly legible) |
| 3 | Tone stack | C22 150p, R36 100k, C23 0.082, C27 0.047, R158 1k, VR2 TRE 250kB, VR4 BASS 250kA, VR3 MID 10kB | 24 mV after the stack (T and B fully clockwise, M fully counter-clockwise) |
| 4 | C17 0.082, R15 1k, Q2 2SK117GR | R13 470k gate leak, R12 220k / R11 220k bias divider (about 13 V, matches the printed +13.2 V), R10 12k source | 24 mV and 27 mV in two boxes around Q2 |
| 5 | Branch to Volume | Clean branch: C4 0.082, R16 100k to the top of VR1 VOL (1MB). Distortion branch: C6 0.082, R17 33k, Q3 stage, Q4 series switch, C15 0.082 into the same node. Section 5 | |
| 6 | Volume + Bright | VR1 VOL 1MB; C5 220p across the top half through the BRIGHT switch, R14 1M bleed | |
| 7 | Q5 2SK117GR | C16 0.082, R30 16 (? probably 1.6k or similar, partly read), R40 1M, R39 4.7k, R35 1k, R60 12k drain load, R50 220k | gate 20 mV, drain 200 mV (+5.1 V DC): gain 10 |
| 8 | Q16 (2SK117GR) + Q15 (2SA970GR) | C52 0.082, R96 1k, R99 1M, R100 220k, R102 100k, R93 100k, R94 8.2k (?), R67 1k, R70 47k, C50 150p, C42 1/50 NP, C53 10/16, D6 | **2.1 V** at the output to MAIN IN / pin 11: gain 10.5 from Q5's drain |

Derived: gain from the volume wiper to the output is 20 mV to 2.1 V, i.e. 105 times (40.4 dB).

The reverb returns through VR6 REV and a JFET switch (Q19) into the same bus; the chorus returns through Q9 / D7 (section 6.5).

### 2.3 Channel 1, 1979 (page 7, "AP-113"). Plain clean channel, +27 V rail.

| # | Stage | Parts as read | Printed level |
|---|---|---|---|
| 1 | Inputs | HIGH: R1 33k; LOW: R2 68k; C1 0.033; R3 1M (+V), R4 22k (ground), R5 220k gate leak; C1 100p (?) to ground | HIGH 22 mV, LOW 72 mV (LOW is 10.3 dB less sensitive) |
| 2 | Q1 2SK117GR | R6 15k drain, R7 1.5k source, C13 47p drain-gate | |
| 3 | Q2 2SK117GR source follower | R8 10k source, drain to the rail | 190 mV after Q2 |
| 4 | C2 0.1, R9 1M | | |
| 5 | Tone stack | C3 0.0015, R10 18k, C4 0.47 (tantalum), C5 0.15 (tantalum), VR1 TRE 50kB, VR3 BAS 100kA, VR2 MID 5kB, R11 100 ohm. Section 3 | |
| 6 | Bright + Volume | C6 0.0015 + R12 22k + SW1; VR4 VOL 100kB | 40 mV at the volume wiper |
| 7 | Q3 2SK117GR | R13 1k, C7 0.033, R16 1M leak, R14 1M / R15 33k bias (about 0.86 V, derived), R17 15k drain, R18 1.5k source, C8 47p, C10 0.033 | 320 mV (gain 8.0, 18.1 dB) |
| 8 | Q4 2SK117GR | R23 1M / R24 27k bias, R25 100k leak, R26 15k drain, R27 1.2k source, C11 47p, C12 0.033 | **3 V** (gain 9.4, 19.4 dB) |
| 9 | Power-amp input | R29 100k in series, R33 100k to ground, R32 100k to the other board (pin 7), MAIN IN jack, C31 0.1, R62 1k, R61 68k | MAIN IN 700 mV |

Derived: HIGH 22 mV to 3 V = 42.7 dB overall, including a stack loss of about 11 to 14 dB.

### 2.4 Channel 2, 1979 (page 7, "AP-114")

Same as CH-1 with these differences: R4 39k, R7 1.8k, Q2 output 160 mV; distortion block with Q5 / Q6 / Q7 / Q8 (section 5); after the volume pot 31 mV; Q3 has R17 10k, R18 820, R16 1M, output 240 mV; R21 15k in series and R22 82k to ground; Q4 has R24 10k, R25 100k, R26 15k, R27 470; output 3 V. Reverb, chorus and vibrato circuits on the same board.

Derived: Q3 stage 31 mV to 240 mV = 7.7 (17.8 dB); R21 / R22 divider 0.845; Q4 stage 203 mV to 3 V = 14.8 (23.4 dB).

## 3. Tone stack (Bass / Middle / Treble)

### 3.1 Topology (both editions use the same Fender-style TMB; I resolved the wiring)

```
 drive D ──Ct──────────────► TT (treble pot top)
 D ──Rslope──► N ──Cbass──► B (= treble pot bottom = bass pot top)
 B ──bass pot (wiper tied to its far end, so it is a rheostat Rb)──► X1
 X1 ──Rseries──► X (= mid pot top)
 N ──Cmid──► X
 X ──mid pot (wiper tied to its top, rheostat Rm)──► ground
 output = treble pot wiper W, loaded by Rload
```
Control direction: Treble at maximum puts the wiper at TT (output straight from the treble cap). Bass at maximum is the largest rheostat value (the bass cap node floats, most bass). Mid at maximum is the largest value to ground (least mid loss). Mid at minimum shorts X to ground (deepest mid cut).

| Part | 1983 CH-2 | 1983 CH-1 | 1979 (AP-113 / AP-114) |
|---|---|---|---|
| Ct (treble) | 150 pF (C22) | 150 pF (C5) | 0.0015 uF (C3) |
| Treble pot | 250kB linear | 250kB | 50kB (VR1) |
| Rslope | 100k (R36) | 100k (R36) | 18k (R10) |
| Cbass | 0.082 uF (C23) | 0.082 uF (C20) | 0.47 uF (C4, tantalum) |
| Bass pot | 250kA | 250kA | 100kA (VR3) |
| Cmid | 0.047 uF (C27) | **0.056 uF (C26)** | 0.15 uF (C5, tantalum) |
| Rseries | 1k (R158) | 1k (R43) | none read (?) |
| Mid pot | 10kB | 10kB | 5kB (VR2) + 100 ohm (R11) to ground |
| Source (drive) | 12k (Q1 drain load R9) | 12k | Q2 source follower, about 100 to 200 ohm (assumed) |
| Load | about 580k (R13 470k + R12 / R11 110k) at Q2's gate | volume pot 1MB (assumed) | volume pot 100kB |

The bass pot in the 1979 drawing: partially read; I assumed the same wiper tie as the 1983 pot. The 1979 pot labels ("TRE 50KB, BAS 100KA, MID 5KB, VOL 100KB") are printed in the PCB legend on page 6 as 50K / 100K / 5K, so the pot values are firm.

### 3.2 Computed responses (nodal solve, 1 V open-circuit drive, source and load as in the table)

Insertion loss = magnitude at the output relative to the drive voltage.

| Setting (T / B / M, 0 = min, 10 = max) | 1983, loss in dB at 50 / 100 / 350 / 1k / 3k / 10k Hz | 1979, same frequencies |
|---|---|---|
| 5 / 5 / 5 | -10.9 / -13.9 / -24.3 / **-21.8** / -12.9 / -8.5 (minimum -26.5 at 518 Hz) | -7.1 / -9.6 / -17.4 / **-13.8** / -7.9 / -6.1 (minimum -17.9 at 436 Hz) |
| 10 / 10 / 10 | -10.8 / -14.9 / -23.5 / -16.8 / -8.0 / -2.5 (minimum -23.5 at 367 Hz) | -7.9 / -11.1 / -15.6 / -9.8 / -3.1 / -0.5 (minimum -15.7 at 303 Hz) |
| 5 / 5 / 10 | -10.6 / -13.6 / -21.3 / -19.5 / -12.1 / -7.9 | -7.0 / -9.4 / -14.1 / -11.6 / -6.9 / -5.3 |
| 5 / 5 / 0 | -11.1 / -14.2 / -27.4 / -24.3 / -13.7 / -9.1 | -7.1 / -9.6 / -22.3 / -17.4 / -9.4 / -7.3 |
| 10 / 10 / 0 (the 1983 printed measurement setting) | -10.9 / -15.2 / -36.7 / -18.1 / -8.4 / -2.7 (minimum -39.1 at 380 Hz) | -7.7 / -10.9 / -29.4 / -11.7 / -3.5 / -0.5 (minimum -29.4 at 354 Hz) |

Mid notch position: 1983 about 500 to 540 Hz (depth -22 dB at M10, -26.5 at M5, -38 at M0, all with T5 B5); 1979 about 400 to 500 Hz (-14 / -18 / -28 dB).

So the stack is a deep mid-scoop passive stack: at flat 5/5/5 the 1983 loss is about 22 dB at 1 kHz and 26.5 dB at the 500 Hz notch (about 15 dB at 10 kHz); the 1979 stack loses 14 dB at 1 kHz.

Corner frequencies of the RC elements (open-pot arithmetic): 1983 treble 150 pF with the 250k pot = 4.2 kHz; bass 0.082 uF with the 250k pot = 7.8 Hz; slope 100k with 0.082 uF = 19 Hz, with 0.047 uF = 34 Hz; mid 0.047 uF with the 10k pot = 339 Hz. 1979 treble 1.5 nF with 50k = 2.1 kHz; bass 0.47 uF with 100k = 3.4 Hz; mid 0.15 uF with 5k = 212 Hz; slope 18k with 0.47 uF = 19 Hz, with 0.15 uF = 59 Hz.

### 3.3 Validation against the printed numbers (this is why I trust the wiring)

The 1983 range figures reproduce when each control is swept 0 to 10 with the other two at maximum:

| Control | Printed 1983 | My solve |
|---|---|---|
| Treble at 10 kHz | 17 dB | 17.2 dB |
| Bass at 50 Hz | 14 dB | 14.0 dB |
| Middle at 350 Hz (treble 10, bass 10) | 13 dB | 13.2 dB |

The printed annotation "180 mV at Q1's drain, 24 mV after the stack" (T and B max, M min) is -17.5 dB; my solve gives -18.1 dB at 1 kHz.

1979: printed 38 / 2 / 18 dB. My solve gives treble at 10 kHz 38.4 dB (bass 5, mid 0), middle at 1 kHz 1.8 dB (treble 10, bass 5), bass at 100 Hz 19.0 dB (treble 10, mid 0). These match within 1 dB, though the exact "other controls" settings used by Roland for 1979 are not printed (the match is with plausible settings, so weaker than 1983).

The existing plugin netlist (`Source/ToneStackTmb.h`, 7-node solver with the mid cap on the mid pot wiper and a 3-terminal mid pot) gives the same response as mine to within 0.3 dB for non-extreme settings (see `Roland_JC120_vs_code.md`).

## 4. Bright switch

| | 1983 CH-1 | 1983 CH-2 | 1979 |
|---|---|---|---|
| Parts | C6 330p, switch SLR-022, R10 1M bleed | C5 220p, switch, R14 1M bleed | C6 0.0015 uF in series with R12 22k, SW1 |
| Placed | across the upper half of the volume pot (top to wiper) | same | same (top to wiper) |
| Pot | 1MB | 1MB | 100kB |
| Printed boost | 5 dB at 10 kHz, volume centre | same | 4 dB at 10 kHz, volume centre |

Derived boost (with a 1 MB load for the 1983 pots; 1 M assumed for 1979), dB at 100 Hz / 500 Hz / 1 kHz / 2 kHz / 5 kHz / 10 kHz:
- 1983 CH-1, volume 50%: 0.0 / 0.9 / 2.5 / 5.0 / 7.2 / 7.8; volume 25%: 0.1 / 1.9 / 4.9 / 8.7 / 12.2 / 13.2; volume 75%: 0.0 / 0.2 / 0.6 / 1.6 / 3.2 / 3.8; volume 100%: 0.
- 1983 CH-2 (220p), volume 50%: 0.0 / 0.4 / 1.4 / 3.5 / 6.5 / 7.5.
- 1979, volume 50%: 0.0 / 0.3 / 0.9 / 2.1 / 3.4 / 3.7; volume 25%: 0.0 / 0.7 / 2.1 / 4.4 / 6.8 / 7.4; volume 75%: 0 / 0.1 / 0.2 / 0.5 / 1.1 / 1.2.

The 1979 value (3.7 dB at 10 kHz) matches the printed 4 dB. The 1983 value is 7.8 dB (CH-1) and 7.5 dB (CH-2) at 10 kHz against the printed 5 dB, so either the wiper load or the pot's real taper differs from my model (not resolved); the shape (zero at full volume, larger at low volume) is robust.

Corner where the cap equals the upper half of the pot at 50%: 1983 CH-1 964 Hz, CH-2 1.45 kHz, 1979 4.8 kHz (22k with 1.5 nF).

The boost is largest at low volume and zero at full volume (the cap is across a section that shrinks to zero).

## 5. Distortion channel (CH-2 only)

### 5.1 1983 (page 3)

Control: VR5 DIST 10kC with push switch (EVH-6PGK20C14), plus a DIST foot-switch jack (pin 16). The chain after Q2:

1. C6 0.082 uF, **R17 33k**, to the gate node of Q3. Gate bias from R21 220k / R20 47k (about 4.6 V node) through R22 1M.
2. **D1 and D2 (1S2473), antiparallel from that gate node to ground**: this is a hard silicon clamp (about 0.6 V each way) at the *input* of the gain stage, not at its output. Signal from R17 is limited to about +/-0.6 V at the gate.
3. **Q3 2SK117GR** (drain R23 15k to +26 V, source R24 4.7k to ground, **C11 100p drain-to-gate Miller cap**). AC source impedance set by C43 1 uF (1/50) in series with R29 330 and VR5 10kC to ground, in parallel with R24 4.7k.
4. C12 0.082 uF, **R28 10k / R27 10k divider** (-6 dB), **Q4 2SK117GR as a series JFET switch** with D3 (1S2473), R26 1M on its gate (switched by the DIST switch line), then C15 0.082 uF to the top of the volume pot. The clean branch (C4 0.082, R16 100k) meets the same node. I could not tell whether the clean branch is muted when distortion is on (?).

Derived numbers (assumed typical 2SK117GR gm of 15 mS; not on the page):
- Q3 gain with the pot at minimum (AC source about 3.2k): about 4.5 (13 dB); at maximum (about 310 ohm): about 40 (32 dB). The printed pot is 10kC, so the pot's low end moves gain a lot in the first third of the knob.
- Miller pole: input node of Q3 sees 33k into 100p x (1 + gain). At gain 4.5: 8.8 kHz; at gain 40: 1.2 kHz. So the top end darkens as Distortion goes up.
- Post-Q3 high-pass: C12 0.082 into R28 + R27 = 20k = **97 Hz**.
- Gate coupling: C6 0.082 into R17 33k alone would be 59 Hz once the diodes conduct; with the diodes off the load is about 1 M (about 2 Hz).

### 5.2 1979 (page 7, AP-114)

TR5 10kC distortion pot with switch SW2 ("DISTORTION"), Q5 JFET stage, Q6 / Q7 JFETs with D4 / D5, Q8 2SC1815GR:
- **Q5 2SK117GR** gain stage: C21 0.033, R41 1k, gate leak R42 1M with R43 120k, R44 1M, drain load R45 22k (supply through R48 22k, C24 47/16, R49 6.8k), **C22 470p drain-gate**, source R46 10k to ground; the variable bypass is R47 100 + C23 1 uF (1/50) + TR5 10kC.
- Derived gain (gm 15 mS, RD 22k): pot at 0 (source about 99 ohm) 133 times (42.5 dB); pot at 10k (source about 5 k) 4.3 times (12.7 dB).
- Then C25 0.033, R50 39k, **Q7** JFET and R51 220k: Q6 and Q7 are JFET **series switches** (gate pulled to a negative rail through R32 / R55 390k and D4 / D5 with C26 / C27 0.1 uF) that select the clean path (Q6, from C2 0.1) or the distortion path (Q7), driven by Q8 2SC1815GR from the foot switch and SW2. I read Q6 / Q7 as switches because the same circuit (Q18 / Q19 with D7 / D8, C71 / C72 0.1, R111 / R114 390k) appears again in the reverb / chorus return mixing. This reading is a topology inference, not printed text.
- No diode clipper in the signal path was found on the 1979 sheet; the clipping is Q5's own saturation (drain swing against the supply) and the following gain stages (Q3, Q4 with the +27 V rail).

## 6. Chorus / vibrato (CH-2 only)

### 6.1 Parts

| | 1979 | 1983 |
|---|---|---|
| BBD | **MN3004** (parts list "020-063 MN-3004 BBD") | **MN3007** (parts list "15219205 MN-3007BBD") |
| Clock chip | MC14011BCP (IC4, NAND oscillator) + MC14013BCP (IC3, divide-by-2 and two-phase clock CP1 / CP2) | **MN3101** (IC4, "clock generator / driver" per parts list) |
| LFO op-amp | NJM4558 (IC5), vibrato oscillator Q33 / Q34 (2SC1815GR x2) | NJM4558DD (IC5), triangle for vibrato and chorus |
| Op-amp for filters | NJM4558 (IC2? shown as MN3004 box with its own pins) | NJM4558DD IC2 |

Stage counts are **not printed on the pages**. From my recollection of the Panasonic datasheets MN3004 has 512 stages and MN3007 has 1024 stages (assumed; the repo has no MN3004 datasheet). The 1024-stage figure for the MN3007 is consistent with `EHX_pages_048_094.md` ("MN3007 1024-stage BBD").

### 6.2 Clock range and delay time (derived)

Printed (page 6, "Checking and adjustment", 1979): clock oscillator sweep range, 15 V peak-to-peak square wave at IC3 pin 1 (CP-2): **vibrato 65 kHz to 130 kHz, chorus 50 kHz to 220 kHz**. Delay of a BBD with N stages = N / (2 x clock).

| Case | Clock | Delay (derived) |
|---|---|---|
| 1979 chorus, MN3004 (512 assumed) | 50 to 220 kHz | **1.16 ms to 5.12 ms** (mean of the two ends 3.14 ms; geometric 2.44 ms) |
| 1979 vibrato, MN3004 | 65 to 130 kHz | **1.97 ms to 3.94 ms** |
| 1983 chorus, MN3007 (1024 assumed), printed clock waveform label "2.5 to 12 us" (period) | 83.3 to 400 kHz | **1.28 ms to 6.14 ms** |

The 1983 label ("2.5~12 us") sits beside a square-wave drawing at IC4 (MN3101); whether it is the clock period at the BBD or the oscillator period before a divide-by-two is not stated. If it were before a divide-by-two, the delay would be 2.56 to 12.3 ms. I used the first reading because it gives about the same delay range as 1979 (1.2 to 5 ms, 1.3 to 6 ms), which is what one expects from a redesign with twice the stages.

Nyquist at the lowest clock: 1979 chorus 25 kHz, vibrato 32.5 kHz, 1983 41.7 kHz.

### 6.3 LFO rate and depth

**Chorus (1979, page 6 and page 7).** Triangle LFO from IC5 (NJM4558): integrator pins 1-2-3 (C111 0.15 uF feedback; R163 150k and R164 150k with the VR1 CHORUS ADJUST 10kB + R166 4.7k shunt at their junction), Schmitt comparator pins 5-6-7 (R162 33k, R165 47k), output through R161 12k to the clock oscillator control (SW3b). Printed adjustment: "Adjust VR1 (rear panel) for **1.3 sec (0.77 Hz)** at R161", level "**25 V pp**", sensitivity: chorus starts to take effect with 100 mV rms at the channel 1 speaker (a threshold for the effect).
- Derived rate range from the schematic values: the junction shunt (R166 plus the VR1 portion, 4.7k to 14.7k) turns the two 150k resistors into an effective integrator resistance of 5.09 M down to 1.83 M. Period = 4 x (R162 / R165) x R_eff x C111 = 2.14 s to 0.77 s: **0.47 Hz to 1.30 Hz**. The printed 1.3 s corresponds to a shunt of 8.1k (3.4k of VR1), inside that range. This confirms the topology reading.
- Triangle amplitude: derived 17.6 to 19.7 V pp for +/-12.5 to 14 V op-amp swing (threshold 0.70 of the swing); the printed 25 V pp does not agree (?), perhaps the supply is higher than I read.

**Vibrato (1979).** Sine oscillator Q33 / Q34 (2SC1815GR x2) with R171 27k, R172 5.6k, R173 10, R174 4.7k (1/2 W), R175 47k, R176 390k, R177 120k, R178 3.9k, C121 47/16, C122 47p, C123 / C124 / C125 0.22/35 tantalum (three-stage RC), **TR8 250k SPEED**, VR7 10kB (depth, "VIB. DEPTH", with R150 1M). Printed: "VIBRATO OSCILLATOR frequency **1.8 to 9 Hz**" with 25 V pp. Q35 / Q36 (2SC1815GR x2) with R181 1M, R182 470k, R183 47k, R184 4.7k, D12, C132 / C133 10/16, R185 10k, R187 1.5M, C134 470p, R186 10k, C135 0.01, R188 15k form the footswitch / selector logic.

**Vibrato and chorus (1983).** IC5 is a triangle oscillator (C82 0.082 uF integrator capacitor, R149 220k, R150 27k, R153 39k, R140 10k, R148 3.9k, R156 / R157 10k, VR7 SPEED 100kA, VR8 DEPTH 100kB with R128 100k, C87 1/50 NP, C88 100/16). SW2 (A)(B)(C) switch VIB / CHORUS. Printed waveforms: **Chorus: triangle, period 1.2 s (0.83 Hz)**, amplitude boxes illegible (?), **Vibrato speed MAX: period 120 ms (8.3 Hz), 15 V pp trapezoid; MIN: period 500 ms (2.0 Hz), 10 V pp triangle**. The depth pot VR8 100kB sets the modulation amount in vibrato; in chorus the speed pot is out of circuit (the divider R148 3.9k / R140 10k sets the speed) and depth is fixed (?). Clock-oscillator timing parts: R152 100k, C101 100p, R151 1k, R153 10k, C102 22p, R141 3.9k, D8 / D9, Q24 2SC2240GR (R142 270k, R139 15k, R145 220k), C73 22p, C74 0.0015, C77 0.0047, C71 1000/16.

So chorus is a fixed-rate, fixed-depth effect (rate 0.77 to 0.83 Hz, adjustable only by the service trimmer on the 1979 board); the front panel has just a VIB / CHORUS switch (SW2) and, for vibrato, SPEED and DEPTH knobs (page 1 parts list: "VIB DEPTH Pot 100kB, VIB SPEED Pot 100kA").

### 6.4 Filters around the BBD (1979, page 7)

- **Input anti-alias, before the BBD**: Q21 2SK117GR gain stage (C81 0.033, R121 150k, R122 560k, R123 47k, R124 1M, R125 10k, R126 3.3k, C82 100p), R127 10k + R128 10k with C83 0.001 uF (feedback) and C84 470p to Q22 2SC2240GR follower (R129 10k): a **unity-gain Sallen-Key low-pass with f0 = 23.2 kHz and Q = 0.73** (derived, assuming the 0.001 uF is the feedback cap and the 470p the ground cap). Then C85 1/50 NP, R130 1k, R131 27k / R132 27k / R133 3.9k bias, R134 100k, C86 0.001, C87 47/16 to the BBD input. Printed levels: 185 mV at Q21, 540 mV after R127, 530 mV at the BBD input.
- **BBD output**: VR9 10kB trimmer ("BBD OUTPUT ADJUSTMENT": adjust for the clock-feedthrough null, seen at R137 on AP-114), C88 0.047, R135 100k, R137 1k, Q23 2SC2240GR (R138 2.2M, C90 100p, R139 10k, R140 2.2k), R136 220k, C89 0.022, Q39 2SK117GR (clock-noise/limiting FET) with D11 and C131 1/50 NP. Printed 480 mV at the BBD output, **2.3 V** after Q23 (gain 4.8, 13.6 dB).
- **Reconstruction filters after the BBD**: two cascaded sections, each three 10k resistors with 0.0015 / 0.0047 / 0.001 uF and an emitter follower (Q24 and Q25 2SC2240GR; R141 to R143, C91 to C93; R145 to R147, C94 to C96; R144 and R148 10k emitter resistors; C97 1/50 coupling). I read C91 and C92 as returning to the follower's emitter (bootstrapped) and C93 to ground. With that reading each section is -3 dB at 6.7 kHz with +2.4 dB peaking near 5 kHz and -26 dB at 20 kHz; the pair is **-3 dB at 6.3 kHz, -24 dB at 10 kHz, -53 dB at 20 kHz** (derived; the alternative all-to-ground reading would give only about 1.3 kHz, which is unrealistically dark, so I kept the bootstrapped reading, flagged as a topology inference).

1983: the pre-BBD filter is a NJM4558 stage (IC2): R115 5.6k with C61 0.0022 and R120 56k with C62 220p (both RC corners are the same 12.9 kHz), R121 27k, R123 4.7k, R125 56k, C64 100p, C65 0.01, C59 0.047; printed 185 mV and 940 mV (gain 5.1 through that stage). BBD output stage (1983): IC3 MN3007; C68 0.047, R130 220k, R129 56k, R126 / R127 10k, C66 100/16; Q23 / Q24 / Q25 section with R135 / R136 / R137 8.2k, C69 0.0082, C70 470p, C60 0.047. Details partly read.

### 6.5 Dry/wet mixing and the "stereo / anti-phase" output

What the pages show:
- **Two independent 60 W power amplifiers**, one per speaker, each with its own input network (R29 100k from its own preamp, R32 100k from the other board, R33 100k to ground). In 1979 the two boards are cross-connected by terminals 5 / 7 (AP-113) and 9 / 11 (AP-114).
- The chorus and reverb returns each go through a JFET switch (1979: Q19 and Q18 2SK117GR with D8 / D7, R31 / R29 100k; 1983: Q9 2SK246GR with D7, R66 47k, R63 1M, R55 / R52 33k, C26 10/16, R56 1M) into one summing node on the CH-2 side. The chorus path carries its own level pot-less return through R31 100k.
- **There is one BBD line and one LFO.** I found no second, inverted LFO and no second BBD. The schematic has no "dimension"-style anti-phase pair.
- External check (forum, not the schematic): a repair thread quotes "normally the dry signal is fed to both amps; in chorus mode one speaker still carries the dry signal while the other carries the chorus signal only". That is consistent with the single wet line on the sheet. I could not confirm from the drawing alone which speaker gets which (the inter-board wiring is too dense), and the SW2 (A) section ("V/B, CH" selector with R79 1M, R75 / R77 / R76 and Q12) selects between dry-only (vibrato) and mixed paths.
- Vibrato: the BBD output replaces the dry sound (100% wet) on both channels (SW2 (A) in the VIB position); the 1.97 to 3.94 ms delay moves by about 2 octaves of clock only if depth is at maximum.

## 7. Spring reverb

### 7.1 1979 (page 7)
- Spring unit **Z-3F** (Roland part 040-001). Pin numbers on the drawing: 7 (red), 8 (blue), 15, 13.
- **Drive**: IC1 **TA-7200P** power amp. Input: R91 47k from the tap node, C41 0.01 uF, R92 100k, C42 100p, C43 47/16, C44 47/25; feedback R93 100k, R94 680, R95 220, C45 0.0068, C46 4.7/25, C47 47/16, C48 47p; output C49 47/16, **R96 10 ohm 1/2 W** to the spring (pin 7); return R97 0.5 ohm. IC1 supply filtered by a 2SC1624Y pass transistor (dashed box), D6 05Z-15A 15 V zener, R98 6.8k, C50 100/25, C51 1/50. Printed: **21 mV in, 1.1 V at the spring drive node** (gain 52, 34.4 dB).
- **Recovery**: spring output about 5 mV (-46.8 dB below the drive) through C61 0.022 uF into R101 47k (high-pass corner **154 Hz**, derived), Q16 2SC2240GR + Q17 2SA970GR (probably; printed "2SA-990GR" or "970GR", (?)) two-transistor gain stage: R102 1k, R103 150k, R104 100k, R105 3.3k, R106 4.7k, R107 1k, R108 5.6k, R109 1k, C62 100p, C63 10/16, C64 250p, C65 0.1, C66 47/16, C67 47/16. Printed **140 mV** after the stage (gain 28, 28.9 dB). Then **VR6 REV 10kB** ("MIDDLE / REVERB" pot in parts list), mix JFET Q18 and R28 / R29 100k into the 3 V bus.

### 7.2 1983 (page 3)
- **Drive**: IC1 NJM4558DD half with C34 47p across R60 47k; two emitter followers (Q8 2SD667C, Q7 2SB647C; D4 / D5 1S2473 bias diodes, R54 2.2k, R57 2.2k, R46 10 ohm 1/2 W, R45 10 ohm 1/2 W); C19 47/16 to spring pin 7; return pin 8 to ground through **R37 0.5 ohm** current sense; R44 680 and R64 680 in the feedback. This is a current drive (the existing `CIRCUITS.md` estimate "gain about 70" is plausible but I could not re-derive it exactly from the legible parts, (?)).
- **Recovery**: spring pin 15 through C35 1/50 to the second IC1 half non-inverting input (R65 100k to ground: corner 1.6 Hz), R72 1k with C40 10/16 from the inverting input to ground, **R68 56k feedback with C39 47p**. Gain = 1 + 56k / 1k = **57 (35.1 dB)** above 15.9 Hz; the 47p across 56k puts the top corner at 60 kHz. Output C38 0.082 uF, R69 1k, then VR6 REV 10kB; mix through the JFET Q19 (R109 220k, R106 1k, R107 1M, R108 220k, R110 12k, C55 0.047).

## 8. Power amplifier (2 x 60 W solid state)

### 8.1 1979 (page 7, each channel on its own board)
Complementary-input, quasi-complementary output stage. Rails ±43 V (printed "+43 V", "-43 V").

| Function | Parts |
|---|---|
| Input | C31 0.1 uF, R62 1k, R61 68k (to ground) |
| Differential pair | Q11, Q12 2SA970GR (PNP, "x2") |
| Voltage amplifier | Q13 2SC1628Y, C34 15p (Miller), C37 100p, R72 100 ohm, R67 1k with a footnote marker "*1" that I could not read (?) |
| Drivers | Q14 2SC1624Y (NPN upper), Q15 2SA814Y (PNP lower); R73 330 ohm 1/2 W, R74 330 ohm 1/2 W; C36 100p |
| Bias network | D1, D3 1S2473, D2 1S2473 (thermal, marked "F"), R71 47 ohm 1/2 W, R69 2.7k, R70 3.9k, R63 47k, R65 2.2k, C35 220/50, C32 22/50 |
| Output transistors | **Q1, Q2 (and Q3, Q4 on the second channel) 2SD736A, all NPN** (note on the sheet: "Power transistors (Q1~4) are all 2SD736A"), on heatsink 048-073 |
| Output stage emitter resistors | **R77, R78 0.3 ohm 5 W** (R101 / R103 0.33 on the 1983 sheet) |
| Base resistors | R75, R76 **10 ohm 1/2 W fusing resistors** ("F"), the protection element |
| NFB | R66 **47k** from the output node to the inverting-side base; R68 **1k** with C33 **10/16** to ground. Closed-loop AC gain = 1 + 47k / 1k = **48 (33.6 dB)**, low-frequency corner 1 / (2 pi x 1k x 10 uF) = 15.9 Hz. Rated 700 mV in gives 22 V out, which needs 31.4; I could not reconcile (the "*1" resistor R67 1k may be part of the loop), (?) |
| Output network | L (air coil) in parallel with R79 10 ohm 1/2 W; Zobel R82 27 ohm 1/2 W with C38 0.01 uF; **R83 0.3 ohm 5 W** in the external-speaker return; R80 47k from the output to the LINE OUT |
| Speaker | pins 20 (output), 21 / 22 (external speaker jack, 8 ohm minimum) |

### 8.2 1983 (page 3)
Same family; rails **±37 V**.

| Function | Parts (CH-1 / CH-2) |
|---|---|
| Input differential pair | Q6, Q7 / Q13, Q14 2SA970GR |
| VAS | Q9 / Q17 2SC2229 with C22 / C51 47p (Miller) |
| Bias spreader | Q11 2SA970GR with Q12 2SC2240GR (complementary pair) / Q21, Q22; R29 4.7k, R46 10k, R47 4.7k, R48 12k (CH-2: R124 4.7k, R122 10k, R119 4.7k, R117 12k) |
| Drivers | Q8 2SD669AC (NPN), Q10 2SB649AC (PNP) / Q20 2SD669AC, Q18 2SB649AC; **C21, C23 150p** (CH-2 C56, C54 150p) Miller caps; R42, R39 330 ohm 1/2 W; C25 0.022 (CH-2 C57) |
| Output transistors | Q13, Q14 / Q28, Q29 **2SD845** (substitutes 2SC3182, 2SC2773, 2SC2564), all NPN |
| Output stage | R45, R44 0.33 ohm 5 W (CH-2 R101, R103); R37, R41 **10 ohm 1/2 W fusing resistors** (CH-2 R91, R97) |
| NFB | **R35 27k** (CH-2 R95) from the output to the inverting base, **R32 1k** (CH-2 R69) with **C16 10/16** (CH-2 C49) to ground: AC gain = 1 + 27k / 1k = **28 (28.9 dB)**, corner 15.9 Hz. Rated 800 mV x 28 = 22.4 V (printed "21.5 V" at the speaker output) |
| Output network | Zobel R50 27 ohm 1/2 W + C27 0.01 uF 200 V (CH-2 R130 / C67); R49 47k (CH-2 R118) line-out feed; R25 100 ohm (CH-2 R81) |
| Supply decoupling | R30 2.2k + C18 47/50, R38 100k, C24 100/50 (CH-1); R88 2.2k + C46 47/50, C63 100/50 (CH-2) |

### 8.3 Protection
No DC-protect relay and no limiting circuit appear on either sheet. Protection is: 10 ohm 1/2 W fusing resistors in each output transistor base ("FU 10", parts list "12559702 FRNB 10 ohm 1/2 W fusible"), a 6 A (100 / 117 V) or T3.15 A (220 / 240 V) secondary fuse (page 2 table: FUSE MGC 6A, CEE T3.15A), and a 10 ohm 1/2 W fusing resistor R151 in the feed to the +15 V and +30 V regulators.

### 8.4 Power supply
1983: transformer secondary rated "37 V x 2 at 2.5 A DC, 3300 uF input"; M4C51 (M4C52) bridge, **C85, C86 3300 uF / 50 V** (one for +37, one for -37), F1 / F2, LED SLP-4B with 3.3k and 1N4003. Regulated rails: +30 V (Q11 transistor with D16 RD30EB4 zener, R19 4.7k, C8 47/35, C13 1000/35), +15 V (Q27 2SD313E pass transistor, D15 RD16EB2 zener, R18 6.8k 1/2 W, C18 100/16, C76 470/50, through D10 1N4003 and R151). The op-amps (IC1 / IC2 / IC5 NJM4558DD) run from +15 V with pin 4 grounded.
1979 (PS-56): transformer #80N, secondary "38 V - 0 - 38 V" (ORN 38V), bridge M4C51, **C146, C147 3300 uF / 50 V** for +43 / -43 V, C144 / C145 1000/16, +27 V from Q41 2SC1815GR (R191 1k 1/2 W, R192 22k, R193 1k 1/2 W, C141 47/50, C142 10/50, C143 47/50), -15 V from D14 05Z-15A with R195 2.7k 1/2 W.

## 9. Speaker load and headroom

**Load.** Each power amp drives one speaker: JC-120 = two 30 cm (12 in) 30-103D, each an **8 ohm** load on its own channel (printed "SP 8 ohm"); external-speaker jack 8 ohm minimum. JC-160: four 25 cm (10 in) 25-33D, 16 ohm each, two in parallel per channel (printed "SP 16 ohm x 2" per amp), i.e. 8 ohm per channel. Speaker sensitivity and impedance curves are not on the pages (?).

**Rated power.** 22 V rms into 8 ohm = 60.5 W per channel, 121 W total (matches "120 W"). The 1983 schematic prints "21.5 V" at the speaker terminal (57.8 W).

**Supply current check.** At 22 V rms (31.1 V peak) into 8 ohm the peak current is 3.9 A and the mean rail current per channel about 1.24 A; two channels give 2.5 A per rail, exactly the 37 V x 2 at 2.5 A rating on page 3. So the rating is both channels at full power.

**Clip estimate (assumed 3 to 5 V lost to the Darlington and 0.3 ohm emitter drop at the peak current):**

| | Rail | Peak output | Vrms into 8 ohm | Power | Over the 22 V rating |
|---|---|---|---|---|---|
| 1979 | +/-43 V | 38 to 40 V | 26.9 to 28.3 V | 90 to 100 W | 1.7 to 2.2 dB |
| 1983 | +/-37 V | 32 to 34 V | 22.6 to 24.0 V | 64 to 72 W | 0.2 to 0.8 dB |

The amp is essentially **at its clipping point at rated power**: about 0.5 dB (1983) to 2 dB (1979) of headroom above rating. Clipping is hard (rail-limited, no output transformer, no soft knee); the only softening is the amp's drive-stage Miller capacitors (150 pF, 100 pF) and the unregulated supply ripple.

Supply ripple (derived, not printed): at 2.5 A mean per rail with 3300 uF and 100 or 120 Hz ripple the sag is I / (f x C) = 7.6 V (50 Hz mains) or 6.3 V (60 Hz) peak-to-peak when both channels play rated power continuously; at one channel it is half.

**Level budget (1983, printed levels).** Rated 22 V rms = +29.1 dBu. The preamp must supply 800 mV at MAIN IN; CH-1 does that from 19 mV on HIGH (gain 32.5 dB), CH-2 delivers 2.1 V (+8.4 dB more than the 0.8 V sensitivity; the difference is lost across the mix network, the exact ratio is not resolved). Estimated input overload of Q1 (12 kohm drain at +13.6 V DC, gain 9.5): about 10 V / 9.5 = 1 V peak (0.75 V rms), 32 dB above the 19 mV sensitivity (assumed, from the printed DC point).

## 10. What I could not read

- Footnote text for "*1" and "*2" on page 7 (they sit at R67 / R82 / C38 and at the speaker-jack inset).
- 1979 NFB network: whether R67 1k contributes to the loop gain (the stated 700 mV sensitivity implies a gain of 31 to 32, my read of R66 / R68 gives 48).
- Exact signal path of the dry feed to the second power amp (inter-board wiring pins 5 / 7 / 9 / 11, and 1983 pins 9 / 10 / 11).
- Whether the clean branch is muted when Distortion is on (1983 CH-2); the exact function of Q4 / D3 (series switch, my inference), Q6 / Q7 (1979 switches, my inference).
- Chorus LFO amplitude (printed 25 V pp does not match the derived 18 to 20 V).
- 1983 pre-BBD op-amp filter structure (IC2 around R115 / R120 / R121 / R123 / R125 / C59 / C61 / C62 / C64 / C65): values listed, topology not resolved.
- 1983 chorus LFO depth setting; the amplitude labels next to the printed triangle.
- BBD stage counts (assumed 512 and 1024) and the 1983 clock label meaning.
- 1979 R30 in CH-2 (Q5 stage, "16" or "1.6k"), R94 (8.2k?), Q17 type.
- Spring unit impedance, speaker sensitivity.

## Method (how the numbers were produced)

Tone stack, bright and LFO / filter numbers come from complex nodal solves in Python (numpy) of the circuits as read above: a 7-node TMB with source and load resistances, the bright network as a parallel branch of the upper volume-pot half, a 3-node bootstrapped RC ladder for the BBD post-filter, and closed-form integrator / Schmitt period for the LFO. The solves were run in the scratchpad, not stored in the project. The stack solve reproduces five independent printed figures (1983: 17, 14, 13 dB range checks and the 180 mV to 24 mV loss; 1979: 38, 2, 18 dB), which is the main evidence that the wiring was read correctly.
