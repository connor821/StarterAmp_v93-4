# Marshall JCM800 Lead Series 50 W & 100 W - preamp (standard and master volume) and power sheets read

- Source files: `jcm800pr.gif` (preamp sheet, 2113x1452) and `jcm800pw.gif` (power sheet, 2089x1428). Hand-lettered 1981 drawings on a printed title border; some handwriting blobbed.
- Preamp sheet title (printed): "Marshall  JCM 800 LEAD SERIES  50W & 100W PREAMP CIRCUIT DIAGRAM - STANDARD & MASTER VOLUME". Section headings (handwritten, underlined): "STANDARD LEAD PREAMP  USED ON 1987, 1959"; "MASTER VOLUME PREAMP  USED ON 2103, 2203, 2104, 2204"; "PHASE SPLITTER CIRCUIT  ALL MODELS".
- Power sheet title (printed): "Marshall  JCM 800 LEAD SERIES  50W & 100W POWER CIRCUIT DIAGRAM - STANDARD & MASTER VOLUME". Headings: "50W OUTPUT STAGE  USED ON 1987 (written '19R7'), 2104, 2204"; "100W OUTPUT STAGE  USED ON 1959, 2103, 2203" (the last digits blobbed). Note: "ALL VOLTAGES ARE AVERAGE ONLY!"
- Revision table (both sheets, bottom right): **1 | 24-4-81 | SG**. Copyright (C) JIM MARSHALL PRODUCTS LTD., BLETCHLEY, ENGLAND.
- Units: capacitors ".022u" etc. in uF, "470p" pF, "50u+50u 500V", "10u"; resistors "K", "M"; handwritten.
- Circled numbers 1-17 on the preamp sheet are test points for the node voltage table below.

## 1. Tube complement
| Pos | Tube | Function |
|---|---|---|
| V1a, V1b | ECC83 | Gain stages 1 and 2 (the sheet draws the standard preamp as V1a = Volume II input, V1b = Volume I input; in the master-volume preamp V1b is the first stage, V1a the second stage) |
| V2a | ECC83 | Standard: mixer/gain stage; MV: third gain stage |
| V2b | ECC83 | Cathode follower |
| V3a, V3b | ECC83 | Phase splitter (long-tailed pair), all models |
| V4, V5 (50 W); V4-V7 (100 W) | EL34 / KT77 / 6550 (USA); printed "EL34  KT77  USA-6550" and "V4-7: EL34 KT77 6550-USA" | Output stage |
| Rectifier | 1N4007 diodes (50 W: 2 per leg in series, 4 total; 100 W: bridge of 4 x 1N4007) | |

## 2. Standard Lead Preamp (used on 1987, 1959)
### Inputs and V1
- Inputs II: two jacks (HIGH, LOW), each through **68K** to the V1a grid; **1M** from the jack node to ground. Inputs I: the same into V1b. (Four jacks.)
- V1a (Volume II channel): plate **100K** to node 5; plate node = test point 2; coupling **.022u** to top of VOLUME II **1M log**; cathode **820** in parallel with **330u** (printed "330u"; "320u"? blobbed) (?) (+ side to cathode), cathode node = test point 1. Wiper -> **470K** -> V2a grid node.
- V1b (Volume I channel): plate **100K** to node 5; plate node = test point 4; coupling **.022u** to top of VOLUME I **1M log**; cathode **2K7** in parallel with **.68u** (printed ".68u", cathode node = test point 3).
- Bright network on Volume I: **.005u** (printed ".005u") from the Volume I top (after the .022u) to the Volume I wiper node, and **470p** (printed "47?p", read 470p (?)) from that wiper node to the V2a grid node, i.e. across the Volume I mixing resistor; the wiper's own **470K** goes to the V2a grid node.
### Mix and V2a
- Mixing: **470K** (Volume II wiper) and **470K** (Volume I wiper) to the V2a grid node.
- V2a: plate **100K** to node 6 (test point 8 = plate); cathode **1K** to ground, no bypass (test point 7 = cathode).
- V2a plate direct to V2b grid. V2b: plate to node 6; cathode (test point 9) **100K** to ground.
### Supply decoupling (standard preamp)
- The drawing shows a **10K 1W** between circled 5 and circled 6 (which side is fed from the HT is not clear (?)), with **50u + 50u 500V** (two electrolytics, common ground) across/after it, then a second **10K 1W** from the circled-6 line to the output "A". Node 5 is on the left of the first 10K (V1 plate loads); node 6 on the right (V2a plate load, V2b plate). Test point 10 is the supply line "A" after the second 10K.
### Tone stack (standard)
- **470p** from V2b cathode to the **Treble 250K lin** top; **33K** from the V2b cathode to node B; **.022u** from node B to treble-bottom / bass-top; **Bass 1M log** (rheostat, wiper tied to top end); **Middle 25K lin**; **.022u** from node B to the Middle wiper. Treble wiper = output "B" (no master volume in this preamp).

## 3. Master Volume Preamp (used on 2103, 2203, 2104, 2204)
- Inputs: **HIGH** jack through **68K** to V1b (first stage) grid, **1M** to ground; **LOW** jack contact wired to the V1b plate coupling: **.022u** from the V1b plate to the LOW switch contact (as on the 2203 CAD sheet), LOW contact line goes to the **470K // 470p** pair then to the volume pot.
- V1b (first stage): plate **100K** to node 5 (test point 4); cathode **2K7** // **.68u** (test point 3).
- **470K** and **470p** in parallel (printed "470K" and "470p") from the coupling node to the top of **PREAMP VOL 1M log**; **.001u** (printed ".00?u", blobbed; read 0.001u) from the pot top to the wiper (bright cap) (?).
- Preamp Vol wiper -> V1a (second stage) grid directly. V1a: plate **100K** (test point 2); cathode **10K**, no bypass (test point 1).
- V1a plate -> **.022u** -> node; **47?K** (series, blobbed "4 7 ?K", read 470K) and **470p** (blobbed "4?p") in parallel from the node to the V2a grid node; **47?K** (read 470K) from the V2a grid node to ground (the shunt). (Divider same as 2203 CAD sheet.)
- V2a: plate **100K** to node 6 (test point 8); cathode **8?0** (blobbed "8 ?"; read 820) to ground, no bypass (test point 7).
- V2b cathode follower: cathode **100K** to ground (test point 9).
- Tone stack: same values as standard (**470p**, **33K**, **.022u**, **.022u**, **250K lin** treble, **1M log** bass, **25K lin** middle).
- **MASTER VOLUME 1M log** after the stack: treble wiper -> top of the pot; wiper = output B.
- Supply: same as standard (10K 1W / 10K 1W with 50u+50u 500V).

## 4. Phase splitter (all models)
- Input "B" through **.022u** to V3a grid (test point 11). **1M** from the V3a grid to junction (test point 16); **1M** from junction to the V3b grid (test point 14); **.1u** (printed ".1u", blobbed) from the V3b grid node to the NFB node (test point 17).
- Common cathode (test point 15) through **470** (printed "4?0", read 470 ohm) to the junction; junction to the NFB node via **10K**; **4K7** from the NFB node to ground; **PRESENCE 22K lin** with **.1u** (printed ".1u") from the NFB node to ground.
- Plate loads: **82K** (printed "82K"? first digit blobbed) and **100K** from node A' (test point 10). Plates test points 12 and 13. **47p** (printed "4?p") between the plates.
- Outputs: **.022u** from V3a plate to "D"; **.022u** from V3b plate to "E".
- Feedback "F": **100K** from F to the NFB node (return from the OT).

## 5. Node voltage table (printed completely; "NOTE: ALL VOLTAGES ARE AVERAGE ONLY")
| No. | 50W Standard | 50W MV | 100W Standard | 100W MV |
|---|---|---|---|---|
| 1 | 0.8V | 2.4V | 1V | 2.6V |
| 2 | 135V | 230V | 150V | 250V |
| 3 | 1.63V | 1.6V | 1.7V | 1.75V |
| 4 | 19?V (195V?) | 192V | 200V | 210V |
| 5 | 250V (?) | 256V | 265V (?) | 280V |
| 6 | 263V (?) | 264V | 275V | 290V |
| 7 | 1V | 0.9V | 1.2V | 1V |
| 8 | 145V | 150V | 160V | 165V |
| 9 | 141V | 152V | 162V | 167V |
| 10 | 295V (?) | 300V | 320V (?) | 330V |
| 11 | 23V | 23V | 24V | 24V |
| 12 | 185V (?) | 190V | 205V (?) | 220V |
| 13 | 175V | 180V (?) | 195V (?) | 210V |
| 14 | 24V | 24V | 25V | 25V |
| 15 | 37.5V | 37.5V | 37.5V | 37.5V |
| 16 | 36.5V (36.3?) | 36.5V | 36.5V | 36.5V |
| 17 | 11V | 11V | 11V | 11V |

Test-point meaning (from the circled numbers on the drawing): 1 V1/second-stage cathode (standard: V1a cathode; MV: second-stage cathode), 2 its plate, 3 first-stage / V1b cathode, 4 first-stage plate, 5 and 6 supply nodes before/after the first 10K, 7 V2a cathode, 8 V2a plate, 9 V2b cathode, 10 HT at A (after the second 10K), 11 PI grid (V3a), 12 V3a plate, 13 V3b plate, 14 V3b grid, 15 PI cathode, 16 PI grid-leak junction, 17 NFB node.

## 6. 50 W output stage (used on 1987, 2104, 2204)
- Two output tubes **V4, V5** (EL34 / KT77 / USA 6550). Grid feed from D (V4) and E (V5); no grid stopper resistors on the 50 W sheet. Grid voltage printed "(EL34) -30V" for each.
- Screens: **1K 5W** per tube, annotated "(USA & Some Export Models Only)", i.e. fitted on those models only; otherwise screens to HT (365 V).
- Bias: **820K** (printed "820K") top and **220K** bottom between D and E (divider; "(150K for USA)" written beside the 220K), **56K** ("(47K for USA)") to **BIAS ADJUST 22K** preset to ground, **15K** to the bias node, **10u** electrolytics (two, + marked) from the bias nodes to ground. Rectifier: **1N4007** and **220K** ("(150K for USA)") from the mains transformer winding "Rd"/"Ro" lower end into the bias line.
- HT: **367V** at the reservoir (printed "367 V"), **100u 500V** reservoir (printed "100u 500V"), standby switch, **T 500mA** fuse, **1471-263 choke** (printed "1471-263 CHOKE"), HT **365V** to V4/V5 plates; plate line near V5 prints "305V"/"365V" (?).
- Preamp supply: "A'": **10K 1W** with **50u** and **50u** 500V electrolytics (two) to ground as drawn.
- Output transformer: "O/P TX 784-159"; primary "Rd" - centre "Wn"/"Wh" - "Bn"; secondary "Gy", "Gn", "Ye", "Or" with a 3-position selector **16 / 8 / 4 ohm** to the output jack; label "OUTPUT 50W RMS". Feedback tapped at "Ye/Or"? (the 16/8 ohm taps) as in the preamp's F line (?).
- Mains transformer "1208-324": HT winding "Rd - Bk - Rd" with the bridge of 2 x 1N4007 per half (4 total) and standby switch; 6.3V heaters "Bn / Bk / Or" leads; primary taps "Ye 240V", "Or 220V", "Bn 120V"; "Power on" lamp; mains fuses "240V/220V T2A", "120V T3A"? (?); mains label "120-240V~ 40/60 Hz 115W".

## 7. 100 W output stage (used on 1959, 2103, 2203)
- Four tubes **V4, V5, V6, V7** (EL34 / KT77 / USA 6550): grid stoppers **5K6** ("5K6") each; screen resistors **1K 5W** each ("1K5W"); no cathode resistors (fixed bias).
- Bias: grid supply "D" and "E" feeding the pairs with "(EL34) -42V"; **220K** top, **220K** bottom ("(150K for USA)" written beside) between D and E; **56K** ("(47K for USA)") to **BIAS ADJUST 22K** preset; **15K**; two **10u** electrolytics; **1N4007** and **27K** (printed "2?K", read 27K (?)) ("(15K for USA)") from the mains transformer lower end. (Same structure as the 50 W.)
- HT: bridge of **4 x 1N4007** with standby switch in the AC leads; reservoir **two 100u 500V electrolytics in series** (printed "100u 500V" x2, drawn on the 470V node and below it with the centre tap point at ground) (?), **470V** at the reservoir, fuse "T 1A" (?), **1471-263 choke**, **468V** plate/screen supply.
- Preamp / PI supply: **10K 1W** and **10K 1W** in series, with **56K 2W** and **56K 2W** in series (each shunted by an electrolytic of ~100u 500V: printed "100u 500V" x2) forming an RC step from the output node; a **100u 500V** electrolytic at the A' end (printed "100u 500V") (?).
- Output transformer: "O/P TX C2668" (printed "C 2 6 6 8"? blobbed, as on the 2203 CAD sheet); taps selector 16 / 8 / 4 ohm; "OUTPUT 100W RMS".
- Mains transformer "T4145": HT winding with centre "Ye" (not used), "Gy", "Bk"; 6.3V heaters "Bn / Gn"; primary "Vi 240V", "Bk 220V", "Rd 120V"; mains fuse "240V/220V T2A", "120V T4A" (?); mains label "120-240V~ 40/60 Hz 375W".

## 8. Not legible / uncertain
- Many hand-written values listed with (?), notably: V1a cathode cap (330u?), the bright caps (.005u / .001u), 470p labels, test-point voltages marked (?), the "100u 500V" reservoir counts in the 100 W stage, the 100 W bias diode resistor (27K?), fuse ratings, OT numbers.
- Which preamp variant feeds which output stage is as stated by the headings.
