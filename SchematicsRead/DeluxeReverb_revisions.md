# Deluxe Reverb revisions: AB763 -> AB868 -> AA1172 -> AA1270 -> '65 Reissue -> '68 Custom

Compiled from the per-schematic read files in this folder (`DeluxeReverb_AB763.md`, `_AB868.md`, `_AA1172.md`, `_AA1270.md`, `_65Reissue.md`, `Custom68DeluxeReverb.md`). Every value below comes from those reads; "(?)" = uncertain in the source read. Items marked **[calc]** are my own arithmetic from the printed values; items marked **[theory]** are standard circuit reasoning that goes beyond what is printed. Not simulated or measured.
Abbreviations: R = ohm, k = kilohm, "BL" = blackface (AB763/AB868), "SF" = silverface (AA1172/AA1270).
Reference-designator note: BL/SF sheets have no designators; the reissues do (R/C numbers given where useful).

## 0. Identity and dating
| Item | AB763 | AB868 | AA1172 | AA1270 | '65 RI | '68 Custom |
|---|---|---|---|---|---|---|
| Drawing | Fender Electric Instrument Co. header, no dwg no. read | Dwg 045450 Rev C, Santa Ana | Dwg 045450 **Rev H**, CBS footer | Dwg 045450 **Rev "6"**, sheet 1 of 2 | Dwg 041285 Rev B, 1993 (Brea CA) | Dwg 7701464000 Rev B, 28-JUN-13 (Corona CA) |
| Sheet voltage tolerance | +/-20 % | +/-20 % | +/-10 % | +/-10 % | +/-20 %, DVM, standby on, specified conditions | +/-20 %, same style |
| Default cap rating note | 400 V | 400 V | 600 V | 600 V | 400 V on most printed caps | 400 V |

## 1. Value-by-value differences
(Same = value identical to the column to the left.)

### 1a. Tubes, heaters, mains
| Item | AB763 | AB868 | AA1172 | AA1270 | '65 RI | '68 Custom |
|---|---|---|---|---|---|---|
| Rectifier | GZ34 | **5U4GB** | 5U4GB | 5U4GB | **5AR4** | 5AR4 |
| V1, V2, V4 (preamp) | 7025 | 7025 | 7025 | 7025 | **12AX7A** | ECC83S-GT (12AX7A) |
| V5 (tremolo) | 12AX7 | 12AX7A | 12AX7A | 12AX7A | 12AX7A | ECC83S-ST |
| V3, V6 | 12AT7 | same | same | same | same | ECC81S-GT |
| V7, V8 | 6V6GT | same | same | same | same | 6V6-S (sheet prints 6V6GT) |
| Heater centre | grounded CT (arrow) | **2 x 100 R** to ground | same | same | 2 x 100 R (R70/R71) | same, plus T5AL250V fuse in heater line |
| Mains fuse | **1 A** slo-blo | 2 A slo-blo | 2 A | 2 A | 2 A fast (100-120 V) | F2A |
| Ground-switch cap | .047 uF (600 V as read) | .047 uF 600 V | .047 uF 600 V | .047 uF 600 V | C30 .01 uF 125 VAC (120 V units) | C30 not used |
Note: 5AR4 and GZ34 are the same rectifier type (outside knowledge, not printed); 5U4GB is a different, higher-impedance tube [theory].

### 1b. Preamp, both channels
| Item | AB763 | AB868 | AA1172 | AA1270 | '65 RI | '68 Custom (Custom / Vintage) |
|---|---|---|---|---|---|---|
| Input network | 68k/68k, 1M | same | same | same | same (R1-3, R13-15) | same + **10k grid stoppers** R117/R119 (A-stages), R118/R120 (B-stages), R121 (mixer), R122 (recovery), R123 (reverb driver) |
| V1A/V2A cathode | 1.5k ∥ 25 uF, +1.3 V | same | same | same | 1.5k ∥ **22 uF**, +1.3 V | 1.5k ∥ 22 uF, **+1.4 V** |
| V1B/V2B cathode (shared) | 820 ∥ 25 uF, +1.3 V | same | same | same | 820 ∥ 22 uF, +1.4 V | 820 ∥ 22 uF, +1.4 V |
| A-stage plate load | 100k (1/2 W by default) | **100k 1 W** | 100k 1 W | 100k 1 W | 100k 1/2 W | 100k 1/2 W |
| Printed plate V (V1A/V1B/V2A/V2B) | +180/+180/+170/+170 | same | same | same | not printed (supply X = +261 V) | not printed (X = +270 V) |
| Tone stack caps/Rs | 250 pF, 100k, .1 uF, .047 uF, 6.8k | same | same | same | same (Vintage-style) | Vintage same; **Custom: C4 .022 uF, 18k (R72) + 6.8k in series (24.8k)** |
| Treble/bass/volume pots | 250k/250k/1M "A" | same | 250k-J/250k-J/1M-J | same | 250k 30A / 250k **10A** / 1M 30A | same as '65 |
| Vibrato bright cap | 47 pF | 47 pF | 47 pF | 47 pF | C10 47 pF 1 kV | **removed (C10 not installed)**; Normal(Custom) never had one |
| V2B output coupling | .02 uF | .02 uF | **.022 uF** | .022 uF | C12 .022 uF | C12 .022 uF |
| Normal path to PI | V1B -> .047 uF -> 220k -> PI-input node (bypasses the mixer grid) | same | same | same | C5 .047 uF + R12 220k (same) | **removed** (C5, R12 not installed): V1B and V2B plates are tied and both go via C12 into the V4B mixer |
| Channel structure | Normal 2 stages; Vibrato 3 (V2A, V2B, mixer) | same | same | same | same | **both channels 3 stages** (A, B, V4B mixer); both get reverb |

### 1c. Reverb
| Item | AB763 | AB868 | AA1172 | AA1270 | '65 RI | '68 Custom |
|---|---|---|---|---|---|---|
| Driver V3 cathode | 2.2k ∥ 25 uF, +8.7 V | same | same | same | 2.2k 1 W ∥ 22 uF, +7.1 V | 2.2k 1 W ∥ 22 uF, +7.4 V; R123 10k stopper |
| Driver plate | +410 V | +410 V | +410 V | +410 V | supply Z +394 V (plate not printed) | supply Z +408 V |
| Driver coupling / grid leak | 500 pF / 1M | same | same | same | C14 500 pF / R26 1M | same |
| Recovery grid | 220k only | 220k only | 220k **∥ 2000 pF** | same | 220k only | 220k + **220 pF (C58)** from pan output + R122 10k stopper |
| Recovery plate | +170 V, 100k | same | 100k 1 W | same | R29 100k | R29 100k |
| Recovery coupling | .003 uF | .003 | .003 | .003 | .0033 uF | .0033 uF |
| Reverb pot | 100k-L | same | same | same | 100k "B" (linear) | same |
| Wet injection | 470k, 220k to ground | same | same | same | R32 470k, R33 220k to ground | **R32 1.5M**, R33 220k |
| Dry/Vib into mixer grid | 3.3M ∥ 10 pF | same | same | same | same | same |
| Mixer cathode | shared 820 ∥ 25 uF (node E) | same | same | same | probably shared with V1B/V2B (?) | own cathode drawn to ground (?) |

### 1d. Tremolo
| Item | AB763 | AB868 | AA1172 | AA1270 | '65 RI | '68 Custom |
|---|---|---|---|---|---|---|
| Osc plate load / cathode | 220k / 2.7k ∥ 25 uF (+2.1 V; plate +270 V) | same | same | same | R43 220k / R44 2.7k ∥ 22 uF (+1.8 V on) | same as '65 |
| Mod plate / cathode | +345 V; 100k ∥ 25 uF (?) +17.0 V | +345; cap read "5/50"?; +17.0 V | +345; 100k ∥ 5 uF/50 V; +17.0 V | same | R45 10M, R46 100k ∥ 22 uF, **+11 V on** | **+18 V on**, otherwise as '65 |
| Intensity pot | 50k RA | 50k RA | 50k RA | 50k RA | 50k "80C" | 50k "30C" |
| Speed pot | 3M RA + 100k | same | same | same | 3M "10C" + R41 100k | same |
| Network caps/Rs | 1M, 1M, 2.2M, .01, .01, .02 | same | 1M, 1M, 2.2M, .01, .01, **.022** | same | R39 1M, R40 1M, R38 2.2M, R37 390k, C20/C21 .01, C22 .022 | same as '65 |
| Where modulated | opto/intensity on the V4A (mixer plate) node: Vibrato path + reverb only | same | same | same | not traced with certainty (?) - long wire from R48 toward PI input | per manual: PI non-inverting input impedance, both channels |

### 1e. Phase inverter and NFB
| Item | AB763 | AB868 | AA1172 | AA1270 | '65 RI | '68 Custom |
|---|---|---|---|---|---|---|
| Input coupling | **.001 uF** | .001 uF | **.01 uF** | .01 uF | C25 .001 uF | C25 .001 uF |
| Grid-bias Rs | **1M / 1M** | 1M / 1M (hand-ink "330K" noted) | **330k / 330k** | 330k / 330k | R49/R52 1M | 1M |
| Bias node / cathodes | +75.5 / +77 V | +75.5 / +77 V | +75.5 / **+90 V** | +75.5 / +90 V | cathodes **+68 V** | **+69 V** |
| Cathode network | 22k, 470 R, 47 R | same | same | same | R50 22k, R51 470 R, R53 47 R | same |
| Plate loads | 82k 5 % / 100k 5 % | same | 82k / 100k | same | R54 82k **1 W** / R55 100k **1 W** | same as '65 |
| Plate V (A/B) | +170 / +180 | +170 / +180 | **+190** / +180 | +190 / +180 | AC only printed | AC only printed |
| PI supply node C | **+325 V** | +325 V | **+375 V** | "+3?5" (last digit unreadable) | Y **+311 V** | Y **+324 V** |
| Global NFB resistor | 820 R | 820 R | 820 R | 820 R | R64 820 R | **R64 1.5k** |
| NFB return | 47 R to ground (node also 22k to PI cathodes, .1 uF to V6B grid) | same | same | same | same (R53 47 R, C26 .1 uF) | same |
| Output coupling | .1 uF | .1 | .1 | .1 | C27/C28 .1 uF 630 V | same |

### 1f. Output stage and bias
| Item | AB763 | AB868 | AA1172 | AA1270 | '65 RI | '68 Custom |
|---|---|---|---|---|---|---|
| Plate / CT / screen V | +415 / +420 / +415 | same | same | same | **+391** (TP37) | **+407** (TP37) |
| Grid stoppers / leaks | 1.5k / 220k | same | same | same | R60,R61 1.5k / R56,R57 220k | same |
| **1200 pF** grid caps | none | **added** | present | present | none | none |
| Screen Rs | 470 R 1 W | same | same | same | R62/R63 470 R 1 W Fp | same |
| Bias voltage (printed) | -35 V | -35 V | -35 V | -35 V | **-37 V** at TP36 (rail -49 V) | **-38 V** at TP36 (rail -50 V); set for 24 mA idle per tube |
| Bias filter cap | 25 uF/50 V | **50 uF/70 V** | 50/70 | 50/70 | C29 22 uF 63 V (wiper) + C36 100 uF 100 V | same as '65 |
| Bias pot network | 10K-L pot + 10k | same | same | same | R58 10k + R59 10k | R58 10k + **R59 22k** |
| Bias series R | 470 R 1 W | 470 R 1 W | 470 R 1 W | 470 R 1 W | R69 22 R (fuse-R) via CR1 1N4003 | same |
| OT / speaker | TR3 125A1A, 8 R (not printed on AB763 sheet) | 8 R | 8 R | 8 R | T2 041318, 8 R | T2 041318, 8 R |

### 1g. Power supply
| Item | AB763 | AB868 | AA1172 | AA1270 | '65 RI | '68 Custom |
|---|---|---|---|---|---|---|
| Reservoir | 2 x 16 uF 450 V parallel | same | same | same | C31 220 uF 100 V in series with C32 47 uF 500 V (+ R65 100k, R66 470k balance) = 38.7 uF [calc] | same |
| Choke | TR2 125C3A | same | same (022707) | same | L1 022707 | L1 022707 |
| Node B cap | 16 uF 450 V | same | same | same | C33 22 uF 500 V | same |
| B -> C resistor | **10k** | 10k | **2.2k 1 W** | 2.2k 1 W | R67 10k 1 W Fp | R67 10k 1 W Fp |
| C -> D resistor | 10k | 10k (hand-ink note) | 10k 1 W | 10k 1 W | R68 10k 1 W Fp | R68 10k 1 W Fp |
| Node C/D caps | 16 uF 450 V each | same | same | same | 22 uF 500 V each | same |
| Printed supply voltages | CT +420; B +410; C +325; osc +270, mod +345 | same | CT +420; B +410; **C +375**; same osc/mod | C unreadable | TP1 +396, Z +394, **Y +311, X +261** | TP1 +415, Z +408, **Y +324, X +270** |

## 2. What the differences do

### Gain
- **'68 NFB 1.5k vs 820**: feedback fraction 47/(1547)=3.0 % (-30.3 dB) vs 47/867=5.4 % (-25.3 dB) [calc]. About 5 dB less global feedback: higher power-amp gain, less damping.
- **'68 channel structure**: Normal path no longer bypasses the V4 stage; Custom gains one more triode stage than the blackface Normal channel; Vintage path equals blackface Vibrato. With a reverb "return" injected 8 dB lower [calc: 220k/(1.5M+220k) vs 220k/(470k+220k) = -7.9 dB wet attenuation at the mixer grid].
- **'68 Custom stack**: 24.8k bottom resistor and .022 uF vs 6.8k and .047 uF: lower insertion loss and shallower mid cut [theory]; 10k stoppers lower the highs a little (Miller RC) and have no DC effect.
- **SF PI input**: .01 uF/330k vs .001 uF/1M [calc]: the 330k lowers the PI input a bit (my estimate about -2 dB: 1M vs 330k loading a ~140k source) [calc, rough].
- **Cathode cap 25 -> 22 uF**, V1A/V2A: small (12 %), negligible except in the lowest bass.
- AB763 vs AB868: nothing in the signal path changes gain.
- **Boost schematic**: V3 cathode "68C" without bypass would cut reverb drive gain (?), see `DeluxeReverb_Boost.md`.

### Compression / sag / headroom
- **Supply impedance and node voltages**: BL +325 V at C, SF +375 V at C (2.2k instead of 10k), reissue +311/+324 V. SF PI plates sit 50 V higher: more PI headroom, harder to overdrive the PI [theory]. The 1200 pF caps (SF) add no compression.
- **Reservoir 32 uF vs 38.7 uF** and 5U4GB vs GZ34/5AR4 (5U4GB has higher internal resistance [theory]): SF/AB868 sag more than AB763 (GZ34) and the reissues (5AR4); the printed no-load-ish voltages are the same (+415/+420), so the difference is not visible on the sheets.
- **Plate +391 ('65) / +407 ('68) vs +415**: lower plate supply and bias -37/-38 V: earlier clipping of the power stage, 6V6 idle 24 mA on '68.
- **Preamp B+ filtering**: BL 10k+10k and SF 2.2k+10k: on node D RC = 12.2k x 16 uF = 0.195 s vs 20k x 16 uF = 0.32 s [calc]; reissue 20k x 22 uF = 0.44 s [calc] (slower "bloom" in the reissue).
- **'68 NFB 1.5k**: less damping of the speaker, more power-amp distortion and sag-driven feel [theory].

### Tone
- **1200 pF PI-to-6V6 grid caps (AB868 onward)**: with a PI source impedance of ~17k (my assumption: 82k∥rp) the first-order pole is around 7-8 kHz [calc, assumption-dependent]; AB763, '65 and '68 have none, so they stay brighter on the top end.
- **.01 uF + 330k (AA1172/AA1270)** vs .001 uF + 1M: high-pass corner 48 Hz vs 159 Hz [calc]: the SF lets more low end into the power stage; the reissues return to the blackface values.
- **2000 pF across the 220k recovery grid leak (SF)**: trims the reverb return highs (effect depends on pan output impedance, not computed); '68 uses a 220 pF to ground and a 10k stopper instead.
- **'68 Custom stack**: voicing change as above; the Vintage channel matches the '65.
- **Bright cap**: all keep 47 pF on Vibrato only; the '68 removes it.
- **Tube types**: ECC83 vs 7025 are low-noise/similar types; no value changes.
- **Reverb**: wet level in '68 is 8 dB lower vs the mix of the others [calc]; AB763/AB868 recovery lacks the 2000 pF.
- **'65/'68 tremolo** modulation point differs from BL/SF (BL: Vibrato channel path only); the Normal/Custom channel is tremolo-modulated in the '68 per the manual.

## 3. Appendix: other related schematics in this batch
- **Deluxe (non-reverb) AB763** (`Deluxe_AB763.md`): B+ node values +420/+410/+340/+230, V1/V2 plates +170, PI .001 uF/1M, NFB 820 R, bias -55 V schematic vs -35 V layout (conflict), 6V6GT, GZ34, 1 A fuse, no reverb.
- **Boost schematic (Dwg 014641)** (`DeluxeReverb_Boost.md`): same as AA1172 apart from volume pots marked "ND", a push/pull switch in the reverb-driver path, V3 cathode resistor without visible bypass (+6.0 V), 1k added after the 220k mix leak, bias cap 80/75, V5 labelled 7025, CSA thermal protector, .047 uF 125 V ground cap.
- **AA1270** vs AA1172: no value differences found; node C voltage unreadable on AA1270.
