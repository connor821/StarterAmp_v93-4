# Marshall schematics: differences between revisions and against what the docs assume

Sources (all in this folder): `JTM45_tremChassis.md`, `Super_PA_1963.md`, `Plexi_1959_pre_power.md` (1959 STD, 1988 CAD sheets, preamp iss 4 / power iss 5), `JCM800_2203_pre_power.md` (2203 STD, 1988 CAD, preamp iss 4 / power iss 6), `JCM800_50_100W_sheet.md` (1981 hand drawing, iss 1, with node-voltage table), `SilverJubilee_2550.md`, `JCM900_power_iss1_iss7.md`, `Plexi_1959SLP_reissue_2002.md`, and `Bassman_5F6A.md` for the Fender side. Values marked (?) are as marked in those files.

Three generations of Marshall drawings appear below and give different values for the same stage, so each comparison says which one it uses:
- **1981 sheet** (`jcm800pr.gif` / `jcm800pw.gif`, "24-4-81 SG"): "Standard" preamp (1987, 1959), "Master Volume" preamp (2103, 2203, 2104, 2204), 50 W and 100 W power stages.
- **1988 CAD sheets** (`1959prem/pwrm.gif`, `2203prem/pwrm.gif`, dated 10-5-88 / 19-5-88 / 7-12-88).
- **2002 reissue** (`1959-01-60-02.pdf`, 1959SLP-01, iss 2, 28-3-02).

---

## 1. JTM45 (1961/1962/1987T trem chassis and 1963 Super PA) against the Fender Bassman 5F6-A

The JTM45 drawings are a close copy of the 5F6-A circuit with changed tubes, supply and a few values.

| Item | Fender Bassman 5F6-A (`Bassman_5F6A.md`) | Marshall JTM45 trem chassis (1961/62/1987T) | Marshall 1963 Super PA |
|---|---|---|---|
| Preamp tubes | V1 12AY7 (one half per channel), V2 12AX7 (gain + CF), V3 12AX7 PI | V1 ECC83 (both halves), V2 ECC83 tremolo osc., V3 ECC83 (gain + CF), V4 ECC83 PI, Q 2G37x tremolo transistor | V1, V2 ECC83 (four channels), V3 ECC83 (gain + CF), V4 ECC83 PI |
| Power tubes / rectifier | 2 x 5881, GZ34 | 2 x KT66 (EL34 + 1N4007 diodes if V7 not fitted), GZ34 (label blobbed) | 2 output pentodes (type not printed), rectifier tube drawn |
| Input network | two jacks per channel, 68k each + 1M | same (68k, 68k, 1M per channel) | same, four channels |
| V1 plate loads | 100k + 100k, plates 150 V | **180K** (V1A) + 100K (V1B), plates 220 V | 100K x 4, plates 210 V |
| V1 cathode | 820 ohm // 250 uF/6 V shared (2.5 V) | 820 // 250 uF shared (1.6 V) | 820 // 250 uF per tube (1.6 V) |
| Volume pots | 1M x 2, bright 100 pF on one channel | 1M x 2, 100 pF on the V1B pot | 1M x 4, no bright caps |
| Mixer | 270k + 270k to V2a grid | 270K + 270K (+ **56 pF** across one) to V3A grid | **470K** x 4 to V3A grid |
| Mixer stage | V2a 820 ohm unbypassed, 100k plate (180 V) | V3A 820 unbypassed, 100K plate (190 V) | V3A 820 unbypassed, 100K plate (165 V) |
| Cathode follower | V2b, 100k cathode (180 V), plate at 325 V | V3B, 100K (190 V), plate 310 V | V3B, 100K (170 V), plate 300 V |
| Tone stack | 250 pF, 56k, .02, .02, 250k treble, 1M bass, 25k mid | **270 pF**, 56K, .02, .02, 250K, 1M, 25K | same as trem chassis |
| PI | 82k / 100k plates, 1M + 1M, 470 + 10k, .1 uF, 47 pF; cathodes 54 V, plates 235/230 V | 82K / 100K, 1M + 1M, 10K + 470, 0.1; cathodes **40 V**, plates 250 V; plate cap blobbed (?) | same; cathode (?) 44 V; plates 240 V |
| Output grid leaks / bias | 220k + 220k, -48 V fixed bias, diode, 8 uF/15k/8 uF/56k | labels blobbed (22K/33K?), bias through diode + 150K, 56K + 15K divider; circled "105V" (?) | 220K (?), same bias network |
| Screens | 470 ohm 1 W each from the 430 V node | 470 1W each, joined and fed by **1K 2W** from the 440 V node | same |
| Feedback / presence | 27k to the NFB line, 5k presence, .1 uF, 10k to PI | 27K, 5K presence, 0.1, 10K (identical) | 27K from the 16 ohm tap, 5K presence, .01 (?), 10K |
| OT | 4 parallel speaker jacks, no impedances | "WHITE 100V" tap and further taps (illegible) | WHITE 100V, BLUE 16 ohm, GREEN 8 ohm, YELLOW, COMMON |
| HT | GZ34 325 V AC; 432 V (2 x 20 uF), 14684 choke, 430 V, 4.7k -> 385 V, 10k -> 325 V | 360 AC, **450 V** reservoir (32 uF?), **20 H** choke, 440 V, 8.2K 1W -> 380 V (16 uF), 10K 1W -> 310 V (16 uF) | 350 AC, 450 V, 20 H, 440 V, 8.2K 1W -> **350** (16 uF), 10K 1W -> **280** (16 uF), extra series resistor (?) to V1/V2 |
| Plate / screen volts (output) | plate 432 V, screens 430 V | plate 430 V, screens 440 V (chart) | plate 430 V, screen 440 V |
| Extras | none | tremolo (V2 phase-shift osc., 330K, 2M, 1M, .01, .01, .022, 27K/50 uF, 150K, 680K, 0.1, 2G37x, foot switch, 50K depth) | none |

---

## 2. 1959 (Plexi) preamp against the 2203/2204 master-volume preamp

Uses the 1988 CAD sheets (1959 STD preamp iss 4; 2203 STD preamp iss 4) and the 1981 hand-drawn "Standard" vs "Master Volume" preamps. Where the 1981 and 1988 sheets differ it is stated.

| Item | 1959 STD preamp | 2203 / 2204 MV preamp |
|---|---|---|
| Inputs | 4 jacks (Inputs I and II, each High/Low), 68k each, 1M | 2 jacks: High through 68k to V1, 1M; **Low jack bypasses V1** (its contact feeds the volume network after V1's plate via C3 .022) |
| Topology | V1a and V1b are two parallel first stages, each with its own 1M log volume, mixed (470k + 470k) into V2a | V1b (first stage) -> **Preamp Vol 1M log** -> V1a (second stage) -> V2a (third stage); one channel, three cascaded gain stages |
| Gain-stage cathodes | V1a 820 // **330 uF** (1981: "330u"? blobbed); V1b **2k7 // .68 uF**; V2a **820 ohm unbypassed** (1981 standard: 1K) | stage 1 **2k7 // .68 uF** (C1); stage 2 **10K unbypassed**; V2a **820 ohm unbypassed** (1981 MV table: 0.9 to 1 V) |
| Plate loads | 100k all (R5, R10, R15) | 100k all (R4, R7, R12) |
| Coupling into the next stage | .022 uF into volume pot, then 470k mixer into V2a | stage 1: .022 into **R5 470K // C4 470p**, Preamp Vol; stage 2: .022 into **R10 470K // C8 470p** with **R11 470K** to ground (a 470k/470k divider) into V2a |
| Bright / treble caps | Vol I: **C7 220p** from pot top to wiper (1981: **.005u**), C6 470p across the 470k mixer R14; **C4 100p plate-to-cathode on V1b** | **C5 1n0** (1 nF) pot top to wiper, R5 470K // C4 470p in series ahead of the pot; **C2 100p plate-to-cathode on stage 1**. (1981 MV: bright cap reads ".001u" blobbed (?)) |
| V2b | CF, plate to HT, 100k cathode | same (R13 100k) |
| Tone stack topology | 470p, 33k, .022, .022 | identical |
| Tone stack pots | 1988: **220k lin / 1M log / 22k lin**; 1981 standard: 250K lin / 1M log / 25K lin | same as the 1959 of the same date (1988: 220k/22k; 1981: 250K/25K) |
| After the stack | straight to the PI via C8 .022 (no master volume) | **Master Vol 1M log** after the stack ("link 'R14'" in the path), then .022 to the PI |
| HT filter | R11 10k 1W + C19 50+50 | R8 10k 1W + C21 50+50 |
| PI | 82K / 100K plates, 220k grid leaks to bias, 47p between plates, 1M + 1M, 470 ohm cathode, 10k, 4K7, presence 22K (lin on 2203), C 0.1 | identical |
| Power sheet | 4 x EL34, 5k6 stoppers, 1k 5W screens, 100k NFB, 22K presence | identical electrically (renumbered); presence marked "22K LIN" |
| Voltage table (1981 only) | Standard column: 100 W V1b plate 200 V, node 5 265 V, node 6 275 V, A 320 V | MV column: 100 W V1b plate 210 V, 5 = 280 V, 6 = 290 V, A = 330 V; stage 2 cathode 2.6 V (10K), plate 250 V; V2a cathode 1 V |

---

## 3. JCM900 100 W power amp (CD0192): issue 1 against issue 7

| Item | Issue 1 (31-10-89, "ALL COUNTRIES", file 900-100.DGM) | Issue 7 (2-7-90, "NOT SEMKO", file CD0192.DGM) |
|---|---|---|
| Revision rows | 1: 31-10-89 CPV | 7: 2-7-90; 6: 16-5-90; 5: 9-3-90 (all CPV) |
| PCB | JMP50A | JMP50B |
| Notes | 4 notes | 7 notes (adds: V3 ECC83 / V4-7 EL34; undesignated components off-PCB; resistors 1/4 w) |
| C9 (across R22 82k) | 220p | 100p |
| C10 (between V3 plates) | 47p | 100p |
| R18 (PI junction to presence line) | 4k7 | 10k |
| C5 (presence feed) | 0.1 uF | MVs 0.1 uF; DRs not fitted |
| Output grid stoppers | R25, R24 **22k**; R27, R26 **4k7** (one resistor per grid) | all four: **1k5 + 2k2 in series** with test point between |
| Screen resistors R33-R36 | 2k2 **5 W** | 2k2 **4 W** |
| R37 | 100 ohm **5 W** | 100 ohm **4 W** |
| FS4 | T3A Picofuse | F3A Type 275 Picofuse |
| R32 | 10k 1W | 10k 1W "Metal Oxide" |
| Top rail | "LINK" jumper | plain wire |
| R17 (feedback to presence line) | 100k | 27k (MVs) / 100k (DRs) |
| C6 across R17 | 47p | 220p (MVs) / 100p (DRs) |
| Output transformer | unnumbered | C3070 |
| Mains | NTC thermistor; internal fuse FS5 (CSA only) "117v/234v TxA"; FS3 TxA; "Mains Transformer" (no number) | no thermistor; FS3 117v T4A / 234v T2A; mains transformer T5826; 0.1u 250v and 100 ohm (MVs) / 22 ohm (DRs) 1/2 W chassis network, 100 ohm 1/2 W; neon type 1610INRB5NKR220V; "220v~" inlet marking |
| Heaters | FS6, FS7 "Heater Fuses - SEMKO & CSA only", 3.15v each side | FS6, FS7 T6.3A 20mm; heater wiring drawing (3 x ECC83, 2 x EL34 "A", 2 x EL34 "B"); "Models: 2100, 2101, 4100, 4101 & 4102" |
| LT winding for preamp PCB | Rd 20v - Bk 0v - Rd 20v | Wh 20v - Bk 0v - Wh 20v |
| Earthing | direct chassis earth plus "Star Earth" | "Star Earth" with Bk/Or, chassis connection via the network above |
| Unchanged | R19 470 ohm, R20/R21 1M, R22 82k, R23 100k, R28/R29 220k, C7 22n, C8 0.1, C11/C12 47n, bias (PR1 22k, R30 56k, R31 15k, R7 56k, C13, C14 10u 100v, C15 47n Class X), FS1/FS2 T500mA with LED fuse-fail, 4 x EL34, 2x 50+50uF 500v, effects-loop network, 4 x 1N4007 bridge, High/Low screen select |

---

## 4. 1959 original (1988 CAD, iss 4/5; 1981 where noted) against the 2002 reissue (1959SLP-01 iss 2)

| Item | 1959 STD (1988) | 1959SLP-01 (2002) |
|---|---|---|
| V1 half 1 (Inputs II / Loudness 2) | 100K, 820 // 330u, .022 coupling, no bright | R7 100K, R2 820R // C2 330u, C3 22n, no bright (same) |
| V1 half 2 (Inputs I / Loudness 1) | 100K, **2k7 // .68u**, .022 coupling, **220p** (1981: .005u) + 470p across the mixer resistor, 100p plate-to-cathode | R8 100K, **R1 820R // C1 680n**, **C4 3n3** coupling, **C5 4n7** bright, **C6 470p** across R9, no plate-to-cathode cap read |
| V2a cathode | 820 ohm **unbypassed** (1981: 1K) | R11 820R // **C7 680n** |
| V2b | CF 100k | CF R12 100K |
| Mixers | 470k + 470k | 470K + 470K |
| Stack | 470p, 33k, .022 x 2, 220k lin / 1M log / 22k lin (1981: 250K / 1M / 25K) | C8 470p, R13 33K, C9/C10 22n, **B250K / A1M / B25K** |
| After the stack | PI via .022 | W4 tap to the effects-card send, W3 return via C11 22n to the PI |
| PI | R19/R21 1M, R18 470, R22 10k, R25 4K7, C13 .1u, R20 82K / R23 100k plates, C16 47p, R26/R27 220k leaks | R16/R19 1M, R17 470R, R20 10K, C13 100n, R22 82K/R25 100K, C15 47p, R23/R24 220K, **no 4K7** |
| Feedback | R24 **100k** from the Ye/Pur tap; presence VR6 22K with C15 .1u; R25 4K7 | R21 **47K** from the **16 ohm** tap; presence **B5K** with C12 100n |
| Output stoppers | 5k6 | **1K5** |
| Screens | 1k 5W | 1K 5W |
| Bias | R32 27k, D1, C18/C17 10u, R29 15K, R28 47k, RV1 22K (reissue: R28 56K, VR1 20K) | R30 27K, D1 1N4007, C18/C17 10u, R29 15K, R28 56K, VR1 20K |
| HT | bridge, T1A, 50+50 reservoir, T3 choke, R30 10k 1W + 50+50 to preamp, R11 10k 1W + 50+50 | bridge, T1A, four 50+50uF 500V, choke, R26 + R27 10K 1W (series) to preamp node, R15 10K 1W, R14 10K 1W, each with a 50+50uF pair; plus 56K/56K 1W + 4 x 50uF string to R33 47K -> +30 V for the effects card |
| Mains | SEL1 240/220/120 V, F1 T2A, lamp on primary, mains transformer T4145 | 230/220/110 selector, fuse T3.15A 230V / T6.3A 110V, 6 V indicator with R31/R32 100R and VR2 100R hum balance |
| OT / outputs | C2668, SEL2 16/8/4, two speaker jacks, DI (R41 2k7, R42 100, R43 560) | two speaker jack pairs, 16/8 impedance selector (4 ohm not clearly drawn), no DI output read |
| Revision | 1988 sheets (preamp iss 4, power iss 5), CPV | iss 2, ECO 2100, 28-3-02, M Ballinger, 5-3-02 |

---

## 5. Differences from what the code assumes

"Doc" cites the file and line of the statement being tested (docs are not changed; code was not opened). "Sheet" says which schematic file/value disagrees. Agreements are listed briefly at the end of each model.

### 5.1 Club Crunch (amp 6: Crunch, Jumped S-part; "50 W British schematic check, 1959 schematic values")

Doc lines: AMP_SOURCES.md:19, CIRCUITS.md:74, MODELED_GEAR.md:109, 138, 144-147, 184, AMP_BUILD_STATUS_b62.md:55.

1. **First stage 2.7k / 0.68 uF (CIRCUITS.md:74; MODELED_GEAR.md:147)** is only one of the two channels. On the 1959 STD sheet (`Plexi_1959_pre_power.md`) V1b (Inputs I, bright) is 2k7 // .68u, but V1a (Inputs II) is **820 ohm // 330u**; in the 2002 reissue both halves are **820R** (330u and 680n bypasses).
2. **"820 ohm / 250 uF channel-1 path" (CIRCUITS.md:74; MODELED_GEAR.md:184)**: the 1959 STD sheets show **330 uF**, not 250 uF (1981 sheet reads "330u" blobbed (?)). 820 // 250 uF is the JTM45 value (`JTM45_tremChassis.md`, `Super_PA_1963.md`).
3. **Stack 250k / 1M / 25k with 500 pF (CIRCUITS.md:74; MODELED_GEAR.md:184)**: no Marshall sheet here has 500 pF; all read **470p** (1959 STD C9, 2203 C10, 1981 sheets, 2002 reissue C8); JTM45/Super PA read **270 pF**. The 1988 STD sheets give **220k lin treble and 22k lin middle** (VR3, VR4), not 250k/25k; 250K/25K appear on the 1981 sheets and the 2002 reissue (B250K, B25K). The doc's "500 pF vs 470 pF judged negligible" is therefore a drawing-vs-sheet issue, not a rounding one.
4. **Slope resistor 33k ✔** (all 1959-family sheets). JTM45 and Super PA use **56K** with .02/.02 (Bassman-style), not 33k.
5. **V2A cathode "820 ohm (1k) / 0.68 uF" (MODELED_GEAR.md:184)**: 1988 STD sheet R12 **820 ohm unbypassed** (no cap drawn); 1981 standard preamp **1K unbypassed**. Only the 2002 reissue has 820R // 680n.
6. **Couplings "0.022 uF and 0.0022 uF" (MODELED_GEAR.md:184)**: 1988 STD has .022 on both channels; the reissue has 22n and **3n3** (not 2n2). Bright cap is not mentioned in the docs for Club Crunch: sheet values are 220p (1988) / .005u (1981) / 4n7 (reissue), with a 470p across the 470k mixer resistor and, on the 1988/1981 sheets, a 100p plate-to-cathode cap on V1b (not in the docs).
7. **Feedback (CIRCUITS.md:74 "could not read"; MODELED_GEAR.md:144 British 50 W family = 100k)**: now readable. 1959 STD (and the 2203 sheets): **R24 100k** to the Ye/Pur tap, **R25 4K7**, **R22 10k**, **VR6 presence 22K with C15 0.1 uF**, C13 0.1 uF from the PI's second grid (2203 renumbering: R22 100k, R23 4K7, R20 10k, VR6 22K lin, C17 .1u). Reissue: **47K from the 16 ohm tap**, B5K presence, 100n. The JTM45 / Super PA family uses **27K** with 5K presence (about 3.7x more feedback than 100k, comparable to the "British lead" 39k family at MODELED_GEAR.md:144). The doc's presence "25k with 0.001 uF" (CIRCUITS.md:107, :109) does not match any sheet (22K lin / 0.1 uF; 5K / 100n).
8. **Grid stoppers 5.6k, screens 1k (CIRCUITS.md:74) ✔** on 1959 STD (5k6, 1k 5W). The 2002 reissue has **1K5** stoppers; the JCM900 power amp has 1k5 + 2k2 (iss 7) or 22k / 4k7 (iss 1) and 2k2 screens.
9. **Supply "bridge, 10k x 50 uF stage" (CIRCUITS.md:74) / "20k / 10k dropping stages with 50 uF" (MODELED_GEAR.md:184)**: 1959 STD: bridge D2-D5, C21 50+50uF, F2 T1A, **T3 choke** (no value), C20 50+50uF on the post-choke node, then **R30 10k 1W** to the X line (PI plate loads and the feed onward), then **R11 10k 1W** with C19 50+50uF for the preamp: a 10k + 10k series chain, not 20k/10k. 50+50uF is drawn as two electrolytics on one node (a dual can) so the filter is 2 x 50 uF per node, not 50 uF (?). The 2002 reissue does have 10K + 10K (R26 + R27) then R15 10K, R14 10K. The doc does not mention the choke. B+ values are not printed on the 1959 sheets; the 1981 100 W sheet prints 468 to 470 V at the plates.
10. **Preamp supply for the "Plexi/JCM crunch stage 300 V / 2.7k" (MODELED_GEAR.md:109)**: 1981 100 W standard preamp table has node 5 = 265 V (?), node 6 = 275 V, V1b plate = 200 V, A = 320 V (?).
11. **Bias "diode and 27k (22k)" (MODELED_GEAR.md:184) ✔** (R30/R32 27k, D1 1N4007; the 1981 100 W sheet prints "27K?" with a USA value of 15K).
12. **Mixers 470k, 68k input resistors, 100k plates, V2b 100k, PI 82k/100k + 47 pF + 1M + 470 ohm (MODELED_GEAR.md:184) ✔**.
13. **The doc's "Orange-ish pushed voicing" (AMP_SOURCES.md:19)** has no Marshall-sheet counterpart; nothing to compare.
14. **Output transformer**: the 100 W sheets print only "C2668" (1988), "O/P TX" (1981) with 16/8/4 ohm taps; no primary impedance on any Marshall sheet here (MODELED_GEAR.md:105 states 3.4k:8 for Lead 50; that figure is not on the 1981 50 W sheet, which prints "O/P TX 784-159").

### 5.2 Master Cranker (amp 15: Stock = 2203 stack and first stage; Modded = Jose mods; Cranked)

Doc lines: AMP_SOURCES.md:28, CIRCUITS.md:35, 57-58, 107-108, MODELED_GEAR.md:105, 138, 144-147, AMP_BUILD_STATUS_b62.md:64, 92 ("check which schematic value is off").

Candidate causes for the "schematic-built Stock disagrees with real 2203 captures" note (AMP_BUILD_STATUS_b62.md:64, 92), from the sheets (`JCM800_2203_pre_power.md`, `JCM800_50_100W_sheet.md`):
1. **Master volume position (CIRCUITS.md:107-108 "master volume pre-tone-stack ... 0.022 uF into the tone stack")**: on both the 1981 MV preamp and the 1988 2203 sheet the **Master Volume 1M log is after the tone stack** (V2b cathode follower drives the stack directly, no coupling cap; stack output -> master -> .022 -> PI). The Jose mod text that "master moves from pre- to post-tone-stack" does not describe the stock 2203 on these sheets.
2. **Stage 3 / V2a cathode: doc "10k unbypassed" (CIRCUITS.md:107)**: sheets give **820 ohm unbypassed** (R9, 1988 sheet; 1981 MV reads "8?0" blobbed (?), 1981 standard 1K). The 10K unbypassed cathode belongs to the **second** stage (R6 / "10K").
3. **"1.5k grid stopper into V2a" (CIRCUITS.md:107)**: not on any sheet. The sheets have **R10 470K // C8 470p** in series and **R11 470K** to ground ahead of V2a (a 6 dB divider, 470 pF bypassing the series 470K) after a .022 uF coupling. This attenuator would materially change the gain structure; it is a plausible reason for a gain mismatch.
4. **"0.022 uF into the V2b cathode follower" (CIRCUITS.md:107)**: V2a plate is **direct-coupled** to V2b's grid on the sheets (no coupling cap).
5. **Gain-pot bright cap "470 pF across its top" (CIRCUITS.md:107, :108)**: the 2203 CAD has **C5 1n0 (1 nF)** across the top of the Preamp Vol, and the **470p is C4 in parallel with R5 470K ahead of the pot**; 1981 MV sheet bright cap reads ".001u" (blobbed (?)). A 470 pF shunt across a 470K series resistor is a different circuit from a bright cap. CIRCUITS.md:57 (hand-drawn 2203 "Arredondo mods") also reads 470 pF bright caps; the printed 2203 CAD value is 1 nF.
6. **Presence "25k with 0.001 uF" (CIRCUITS.md:107)**: sheets give **22K lin with 0.1 uF** (and R23 4K7, R20/R22 10k). MODELED_GEAR.md:138's "5k presence pot, 4.7k and 0.1 uF" matches the 4K7 and 0.1 uF but not the pot (22K).
7. **PI "100k plates, 100k tail" (CIRCUITS.md:107)**: sheets give **82K / 100K plates**, **470 ohm common cathode with 1M/1M grid resistors, 10K and 4K7 to the NFB node**, 47 pF between the plates. MODELED_GEAR.md:138's "82k / 100k plate loads, 1M" ✔ for the kit schematic.
8. **Tone stack "250k, 1M, 25k, 33k, 470 pF / 22 nF" (CIRCUITS.md:35, :107; MODELED_GEAR.md:105)** ✔ on the 1981 sheets; the **1988 2203 STD CAD has 220k lin treble and 22k lin middle** (VR3, VR4) with 470p / .022 / .022 / 33k. The stack therefore depends on the revision.
9. **Feedback 100k from the 8 ohm tap (CIRCUITS.md:107)**: sheets: 100k (R22 / R24) from the **"Ye/Pur"** tap, next to the 8 ohm terminal (?), not labelled "8 ohm".
10. **Supply (MODELED_GEAR.md:138 "10k/2 W with two 50 uF, 0.5 s, about 424 V" for the 50 W)**: the 1981 50 W sheet prints **10K 1W** with 50u + 50u 500V at A', reservoir **100u 500V**, HT **365-367 V** (plates 365 V, screens 365 V), a **T 500mA fuse** and a **1471-263 choke** before the OT; the 100 W stage prints **468-470 V**. Neither is 424 V, and the dropping resistor is printed 10K 1W (doc: 10k / 2 W). For the 100 W stage MODELED_GEAR.md:146 uses 495 V / 0.40 A / 90 ohm and 15k x 10 uF against 468-470 V and a 10K + 10K chain with 50+50 uF cans on the sheets. No choke mentioned in the docs (MODELED_GEAR.md:146 "50 W British 10k x 50 uF").
11. **Grid stoppers "5.1k, screens 1k/5 W" (MODELED_GEAR.md:138)**: 1981 100 W stage: **5K6** stoppers and **1K 5W** screens; the 50 W stage has **no stoppers drawn** and the 1K 5W screens are noted "USA & some export models only". The 2203 1988 CAD: 5k6 / 1k 5W.
12. **Bias**: sheets: -30 V (50 W, EL34) and -42 V (100 W) grid voltages printed; bias adjust 22K, 56K (47K USA), 220K+220K (820K + 220K on the 50 W: "820K" (?) top), 15K, 10u x 2; not mentioned in the docs.
13. **Voltage table check (MODELED_GEAR.md:105 "V1b plate 196 V against 190 V, cathode 1.61 V against 1.63 V")**: the 1981 table (test points 3 = V1b cathode, 4 = V1b plate) prints 50 W Standard **1.63 V / "19?V"** (blobbed), 50 W MV **1.6 V / 192 V**, 100 W Standard 1.7 V / 200 V, 100 W MV 1.75 V / 210 V. The doc's pairing of 190 V with 1.63 V matches the 50 W Standard column, not the MV column (1.6 V / 192 V) of the MV amp it models. The difference is small (about 2 V and 0.03 V) but the "published" figures are not both from the MV column.
14. **First stage 2.7k / 0.68 uF, 100k plate, 68k, 1M, .022 ✔** (stage 1 on MV sheets).
15. **Cathode follower 100k ✔; 33k slope ✔**.

### 5.3 Plex Appeal (amp 23: Plexi 100 W-style, Stock/Jumped/Modded)

Doc lines: AMP_SOURCES.md:36, CIRCUITS.md:103-110, MODELED_GEAR.md:200, AMP_BUILD_STATUS_b62.md:72.

1. **"Plexi-style, stock ... master volume after the mixer and before the tone stack" (CIRCUITS.md:109)**: neither the 1959 STD sheets nor the 2002 reissue has a master volume; the mixer output goes into V2a, then V2b CF, then the stack, then the PI.
2. **Mod-lab "1M master, 0.022 uF" ahead of the stack** (same line): not on the sheets (no master; V2a -> V2b direct, V2b -> stack direct).
3. **Bright cap "5 nF / 500 pF" (CIRCUITS.md:103, :109)**: 1981 sheet **.005u + 470p** (5 nF ✔); 1988 STD **C7 220p** (no 5 nF) + C6 470p; reissue **C5 4n7 + C6 470p**.
4. **Mixers "478k / 470k" (CIRCUITS.md:109)**: **470k / 470k** (R13, R14 / R9, R10).
5. **Stack "250k / 1M / 25k ... 470 pF / 22 nF / 22 nF, 33k" (CIRCUITS.md:109; MODELED_GEAR.md:200)**: matches the 1981 sheet and the reissue; the 1988 STD has **220k lin / 22k lin**. The 470 pF in :109 and :200 agrees with every sheet, but CIRCUITS.md:103 reads **500 pF** for the same stack (no sheet has 500 pF). The "cathode follower source of about 1k" (MODELED_GEAR.md:200) is an output-impedance figure and does not conflict with the sheets' **100k cathode load** (R16 / R13 / R12).
6. **Presence "25k / 0.001 uF" and "negative feedback 47k from the 8 ohm tap" (CIRCUITS.md:103, :109)**: 1959 STD (1988, 1981): **100k**, 22K lin presence, 0.1 uF, 4K7; only the **2002 reissue** has 47K (from the **16 ohm** tap, with a B5K presence pot and 100n). The JTM45 / Super PA: 27K with a 5K presence pot.
7. **"V1a / V1b / V2a cathodes 820 ohm (V1a bypass 0.68 uF)" (CIRCUITS.md:103)**: 1959 STD: V1a **820 // 330u**, V1b **2k7 // .68u** (so the 0.68 uF goes with 2k7 on V1b, not with 820 on V1a), V2a 820 unbypassed. Reissue: Loudness 2 half 820R // 330u, Loudness 1 half 820R // **680n**, V2a 820R // **680n**. Also CIRCUITS.md:109 "two halves of V1 (shared 100k plate load)": the sheets have **separate** 100k plate loads per half (R5 / R10 on the 1959 STD, R7 / R8 on the reissue). The "V1b to V2a coupling 0.022 uF" (CIRCUITS.md:103) is not the 1959 topology: V1b feeds V2a through its volume pot and a 470k mixer resistor, and V2a is direct-coupled to V2b.
8. **"V1 and V2 plates 100k" ✔; "V2b cathode follower 100k ✔"; "33k slope ✔"; "phase inverter 12AX7 long-tail ✔".**
9. **Plexi Jose-mod list (CIRCUITS.md:110)**: no schematic evidence on these sheets; the 2002 reissue's channel-2 bypassed V2a (C7 680n) matches the "bypass" idea only loosely.
10. **Low-gain Plexi / JTM 45 voicing (AMP_BUILD_STATUS_b62.md:72, :94 "new voicing")**: the docs contain **no JTM45 schematic values**, so any new voicing is untested against a drawing. Values to use from the JTM45 sheets (`JTM45_tremChassis.md`, `Super_PA_1963.md`): ECC83 pair, V1 cathode 820 // 250 uF shared, plate loads 180K / 100K (trem chassis) or 100K x 4 (Super PA), .02 couplings, 1M volume pots, 270K (trem) or 470K (Super PA) mixers, V3A 100K / 820 unbypassed, V3B CF 100K, **stack 270 pF / 56K / .02 / .02 / 250K / 1M / 25K**, PI 82K/100K / 1M / 470 / 10K, **KT66 (or EL34) with 470 ohm 1W + 1K 2W screen feeds, 27K feedback with 5K presence and 0.1 uF**, plates 430 V, screens 440 V, bias circled 105 V (?), 450 V reservoir, **20 H choke**, 8.2K / 10K 1W dropping stages with 16 uF. That differs from the Plex Appeal stack (33k slope, 470 pF) and feedback (47k / 100k, 25k presence).
11. **Brit Eighteen (AMP_BUILD_STATUS_b62.md:82)**: the doc says it is not listed in AMP_SOURCES.md (not checked) and that the JTM 45 captures are mapped to it as well as to Plexi. None of the sheets read here is a Brit Eighteen circuit, so no value can be compared; the only JTM45 values on hand are those in item 10.

### 5.4 Slow Burn (Soldano SLO-100-style) / British lead family

Doc lines: AMP_SOURCES.md:16, CIRCUITS.md:38, 73, MODELED_GEAR.md:144-145. There was no Soldano sheet among these files when this was written; it is now filed as `Soldano_SLO100.md` (b66). Cross-checks:
1. **British lead family 39k feedback "about 2.6 times the 50 W" (MODELED_GEAR.md:144)**: Marshall 100k (1959 STD, 2203, 2550, JCM900 iss 1 100k), 47k (2002 reissue), **27k** (JTM45, Super PA, JCM900 iss 7 "MVs"). A smaller resistor means more feedback, so the Marshall 27k is stronger than the SLO's 39k, but the tap differs (CIRCUITS.md:73 says 39k from the 4 ohm tap; the Marshall 27k comes from the OT secondary top / 16 ohm tap), so the two are not directly comparable. The doc's SLO presence (25k with 0.1 uF, CIRCUITS.md:73) compares with 22K lin / 0.1 uF (1959, 2203, 2550) and 5K / 0.1 uF (JTM45, reissue 5K / 100n).
2. **Slope 47k / stack 250k, 1M, 25k with 470 pF / 0.02 uF (CIRCUITS.md:38, :73)** vs Marshall **33k** slope with 470 pF / .022 (1959/2203/reissue) and **56K / 270 pF** (JTM45).
3. **2.2k grid stoppers, 475 ohm screens (CIRCUITS.md:73)** vs JCM900 iss 7 **1k5 + 2k2** (3.7k) stoppers and **2k2 4W** screens (iss 1: 22k/4k7, 2k2 5W); 1959/2203 5k6 / 1k 5W; reissue 1K5 / 1K 5W.
4. **Supply "497 V, 2 x 200 uF, choke, 10k x 40 uF"**: no Marshall sheet shows these values (1959/2203 B+ not printed; 1981 100 W 468-470 V with a 1471-263 choke). No comparison possible.

### 5.5 Silver Jubilee 2550 (no model cites it)

No amp row in AMP_SOURCES.md or CIRCUITS.md mentions the 2550. For reference when a "British 50 W / lead" family is applied: 2550 has **100K NFB (R44)**, **R20 4k7**, **VR7 22K LIN presence with .1u**, 5k6 stoppers, **1K5W screens** (reads 1K5W: ambiguous 1.5k / 1k 5W (?)), **S3 HI/LOW switch** (screen resistors to the HT node in H, to the plates in L, as drawn (?)), 82K/100K PI with R17 10K to the NFB node, and a cascaded preamp (V1A 2k7 // .68u, V1B 1k5 // 10u, V2A 1K5, V2B 470) with LED/1N4007 clippers; the tone stack uses 4n7 / 220p / 100k / 10k / .1 / .01 / 470p / 2n2 caps and 220K lin / 100K log / 1M log tandem pots, not the 33k/470p Marshall stack. If a future voicing uses it, the doc's JCM800 stack values do not apply.

### 5.6 Other places the docs touch the same sheets (for completeness)

- **MODELED_GEAR.md:105 "Lead 50 ... 3.4k:8 ohm output transformer"**: the 1981 50 W sheet prints "O/P TX 784-159" and 16/8/4 ohm taps; no primary impedance is printed.
- **MODELED_GEAR.md:146 "50 W British 10k x 50 uF; 420 V / 0.30 A / 100 ohm"**: sheet 365-367 V (50 W). Docs' 424 V comes from the kit schematic.
- **MODELED_GEAR.md:144 "American combo 820 ohm feedback"** is Fender and unaffected.
- **TRANSFORMERS.md:6 "1750D (Marshall-style 100 W)"**: the sheets print OT part numbers C2668 (1959/2203 STD), C3070 (JCM900 iss 7), 784-159 (1981 50 W), 784-139 (2550); no impedance printed on any of them.
