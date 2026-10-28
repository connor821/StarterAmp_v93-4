# misc/schematD.png - identification and values

## What it is

`misc/schematD.png` (2115 x 934) is the **UREI / United Recording Electronics Industries "Solid State Limiting Amplifier, Model 1176LN", Rev "D"**, "Effective for S/N suffix D", dated /-9-70 (sic, date partly illegible; same format as the Rev K sheet). Hand-written headline "REV D" and "SER# 1239 -> 2331"; hand notes: "No significant changes between Rev C and Rev D" and "1176 D includes the RFK circuitry on the motherboard" (RFK = the L-N / low-noise module of Rev C). The serial range 1239-2331 matches Rev D in the revision log (manual p-45) and it is the same drawing as `urei/p-52.png` (Rev "D"), but at higher resolution. Not a different compressor: it is the older (transformer-input, discrete meter driver) version of the same 1176LN circuit that `misc/1176sch.png` shows in Rev K form.

Notes printed on the drawing: (1) Q1 and Q11 selected; Q5/Q6 matched pair; Q7, Q8, Q9, Q10 hFE 250 or above; Q12 and Q13 identical hFE +/-10 %. (2) unless otherwise noted: resistors in ohms, 1/2 W, 5 %; capacitors in microfarads; switches normally open. (3) parts located at the front panel are marked F.P. (4) "*" symbol indicates factory selected value.

## Differences from Rev K (the schematic in `1176_schematic_notes.md`)

| Block | Rev D (this drawing) | Rev K |
|---|---|---|
| Input | rear terminals INPUT +/C, GND, OUTPUT +/C, plus REMOTE METER terminals (BLU, GRN). Input transformer T1 "UTC O-12 or equivalent" (secondary taps 6, 7, 8; primary taps 1-5), preceded by a "T" attenuator (R1A, R2, R3, pot R18 "F.P." - values illegible (?)); R4 270 ohm across the secondary | IC2 NE5532 differential amp, no transformer; no remote meter terminals |
| Series/shunt attenuator | R5 27k series, Q1 "VVR" URET 13-0027 shunt; C28 50 uF / 6.4 V and R10 10k source network; R6 2.2M with C3 200 pF, R7 1.2M (?) with C4 200 pF and C12 .22 around the gate | R6 27k, Q1 BF245A, C2 100 uF, R9 2.2M, R7 2.2M, C27 .22 |
| Preamp | Q2, Q3 2N3391A, "Q14" 2N3391A emitter follower (numbering differs from Rev K), R17 6.8k, R15 6.8k feedback (Rev K R21 7.5k), R86 100 ohm trimpot (Rev K R16), C4 80 uF/25 V, C5 80 uF/25 V, C6 27 pF, C7 1 uF/40 V (output coupling) | Q2-Q4 BC107/2N3391A, R21 7.5k, R16 100 ohm |
| Output pot | R23? "250K/2W F.P." (value read as 250k (?); Rev K 100k 2 W) | R27 100k 2 W |
| Line amp | Q4 2N3391A (gain stage, collector 68k, bias 2.7M/2.7M), Q5 2N3707, Q6 "2N6053"-type (?) emitter-follower driving T2 directly (no C15 470 uF), C10 1 uF/40 V, C11 10 pF, C12 5 uF/25 V (?), C13 210 pF (?), C14 .033 (?), CR1 1N914 or equiv, R35 560 + C16 .0047 Zobel across T2, R32 33 (?) | Q5 BC107, Q6/Q7 BC560 PNP, Q8/Q9 BD517/BD518 Darlington complementary output, C15 470 uF to LL5402, R84 1k |
| Output transformer | T2 UA-5002 (UREI part), many tapped windings: GRN, BLK (primary), BLU, YEL, ORG, RED, WHT/BLK, WHT/RED, SLT, VIO (feedback / secondary / meter taps) | T2 Lundahl LL5402 (163-12614), 2 pins primary, 2 pins secondary |
| Ratio bank | same eight-switch structure; labels "4:1, 8:1, 12:1, 20:1" printed above S1A..S4A / S1B..S4B (this is the direct confirmation of the button order); divider R21 56k, R20 56k, R19 68k, R18 56k, R22 47k to ground; "UA4706 T+C" selected block; bias ladder R61 470, R62 560, R63 1.5k, (VIO node) R64 1.5k to rail, R58 150; no R46 10M (added in Rev E) | R26 56k, R25 68k, R24 56k, R2 56k, R23 47k; ladder R42 470, R43 560, R44 1.5k, R45 2.2k, R87 150, R46 10M |
| Control amp | Q7, Q8, Q9, Q10 2N3707: R54 1M (bias), R57 470k, R58 47k, R40 2.4k, R41 270, R39 47k, R42 180k, R43 47k, R45 390k (?) with selected "T+C" part R45/R46, R46 47k, R47 47k, R48 8.2k, R49 2.4k, R50 180, R51 4.7k (supply), C17 1 uF/40 V, C18 50 uF/50 V, C19 6.8 uF/35 V, C20 6.8 uF/35 V, C21 50 uF/25 V, R52 47k, R53 47k, CR2 and CR3 FD333 | Q12-Q15 BC107/2N3707: R60 1M, R61 470k, R62 47k, R63 4.7k, R64 2.4k, R65 270, R66 182k 1 %, R67 38.3k 1 %, R68 47k, R69 44.2k 1 %, R70 4.7k, R71 7.68k 1 %, R72 2.4k, R73 180, R74 47k, R75 47k, C18 47 uF, C19, C20 6.8 uF, C21 100 uF |
| Attack | C17 .022 across R54 470; R55 25k 1/2 W | C22 .022, R76 470, R77 25k |
| Release | R56 5M 1/2 W, R57 270k | R78 5M, R79 270k |
| Q bias | R59 2k "P.C. Board trim pot", R60 3.9k, R55 10k (?), R65 (T+C) selected | R81 2k, R82 10k, R83 3.9k |
| GR meter driver | Q11 "URET 13-0027" FET, Q12/Q13 2N3707 differential pair, R74 2.7M "null test", R75 2k P.C.B. zero adjust, R71 2k front-panel tracking pot, R64 1.5k, R69 1.5k, R70 4.7k, R72 1.2k (?), R73 680 (?), R76 8.2k (?), R77 8.2k (?), R78 3.6k (?), R79 15k to +30 V, R65 3.9M, R66 10k, R67 3.9k | Q10 BF245A, IC1 TL071, R48 3.9M, R52 10k, R53 15k, R54 100k, R55 2k, R56 10k, R57 470k, R58 100k, R59 3.6k |
| Power | T3 PT-983 with strapping for 120 V / 240 V, F1 3AG 1/4 A slo-blo, lamps DS1,2 #1819, CR4-CR7 1N4003 rectifiers, R80 75 ohm 3 W, CR8 30 V 1 W zener (1N5368A or equiv) for +30 V, CR9 10 V 1N4740 zener for -10 V, C24/C25 2000 uF/50 V (2 x), C24/C25 2000 uF 10/50 V (negative), R61 1k, R62 1k | T3 16-11754, 7824 regulator VR1, R87 1100, R89 220, CR6 10 V zener |
| Voltage labels | "+30 V @ 50 mA, ripple <0.25 mV with load" (?) and "-10 V ripple <0.15 mV with load" (?) | same text; static voltages added to Rev K only |

## Values read (Rev D) that matter to the model

Same VVR / sidechain principle as Rev K; the attack/release network is identical (R54 470 with .022 uF, R55 25k attack pot; R56 5M, R57 270k release; R59 2k Q-bias trimmer). Compression ratios 4:1, 8:1, 12:1, 20:1 are set the same way: B-section divider 56k/56k/68k/56k with 47k to ground, A-section bias ladder 470 / 560 / 1.5k with 150 ohm. So the time constants and ratios derived from Rev K apply equally to Rev D. Rev D's output stage is a transformer-coupled class-A / emitter-follower type with a UREI UA-5002 transformer carrying feedback windings, rather than the Rev K push-pull.

Readability: this scan is noisy and the small bold values are partly illegible; resistor numbers on this sheet are renumbered relative to Rev K, so a cross-reference by number between the two sheets is not valid. The Rev D numbers marked (?) above are my best reading.
