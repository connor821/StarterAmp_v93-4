# Teletronix / Universal Audio LA-2A: circuit and spec notes

Written from the page images (not the text extractions alone). Nothing here is applied to code.

## Sources and how they differ

| Tag | File set | What it is |
|---|---|---|
| **UA** | `la2a/manual-01..21.png`, `manual.txt` | Universal Audio LA-2A reissue "User's Guide", part LA2A-M01, rev 1.3, copyright 2000 (17 printed pages, 21 scanned pages). Has the specifications, calibration, theory of operation (Figures 3 to 7) and an appendix on the 1176 and LA-2A with engineer quotes. |
| **UA = doc70f05** | `la2a/doc70f05-01..21.png`, `doc70f05.txt` | **Identical to UA.** The text files are byte-for-byte the same, and all 21 page images compare as pixel-identical (ImageMagick `compare -metric AE` gives 0 on every page; only the PNG metadata differs). So "doc70f05" is a second copy of the same UA manual, and it adds nothing. Treat them as one source. |
| **TT** | `la2a/preservation-01..10.png`, `preservation.txt` | Original Teletronix Division of Babcock Electronics Corp. instruction manual, "LA-2A (3-66)" (Costa Mesa, California), plus an addendum "LA-2A (11-66)". 10 scanned pages (OCR text is poor; I read the page images). Page 9 is the full schematic, "Schematic - Leveling Amplifier Model LA-2A", drawing S-1014-1 (date block 12-21-61, a revision date that reads 7-19-64). This is the only primary schematic in the set. |
| (extra, not in the task list) | repo `reference/compressors/sources/Teletronix_LA-2A.pdf` | A 2018 redraw by a Hungarian hobby site (elektroncso.hu, "Rev 1.0"). I used it only as a cross-check on topology, and it is NOT reliable for values (see "Redraw cross-check" at the end). |

Differences between UA and TT:

- TT is the 1966 original. It has the stage-by-stage schematic and an addendum (after serial number 572) that introduces the **leveling slope switch** (Limiter / Compressor). UA has the modern Limit / Compress switch described in words only and no switch schematic.
- UA's "Specifications" are a trimmed copy of TT's. UA drops the attack number (says only "Very fast attack time"), drops the 50/150/250/600 ohm taps (says 600 ohm balanced), drops the 0.5 percent at 30 dB gain reduction figure, and relabels the Gain knob "(Output level)" where TT says "(input level)".
- UA's theory text adds the CdS "two-stage decay" description with numbers (40 to 80 ms to half, then up to several seconds), and calls the circuit "feed-back style". TT's block diagram and text show the side-chain fed from the input transformer, ahead of the attenuator (see the open question below).
- UA's schematic figures (5, 6, 7) are redrawn; the TT schematic is the original drawing.

## 1. Signal path, stage by stage (values as printed; "(?)" = not clearly legible)

Tube complement, both docs: **(2) 12AX7A, (1) 12BH7A, (1) 6AQ5(A)**. TT also: power 115/230 V, 50/60 Hz, 35 W; fuse 3AG 3/8 A slow-blow (115 V) or 15/100 A slow-blow (230 V).

Tube reference designators on the TT schematic: V1 = 12AX7A (two triodes, V1A/V1B, the gain stage), V2 = 12BH7A (two triodes, V2A/V2B, the output "totem pole"), V3 = 12AX7A (control amplifier), V4 = 6AQ5A (EL panel driver).

### Input
- Inputs on TB1: terminals 1 to 5 = 600, 250, centre tap (CT), 250, 600 ohm (UA page 2: 1 = +600, 2 = +250, 3 = centre tap, 4 = -250, 5 = -600). TT lists input impedance 50/150/250/600 ohm balanced or unbalanced; UA lists 600 ohm balanced.
- A "jumper plug, pad or pre-emphasis network" socket J1 sits in the input (TT schematic).
- Input transformer: **UTC HA-100X**. Secondary taps feed (a) the optical attenuator and (b) the side-chain.
- **R5 68K** hangs off the transformer's other secondary terminal (pin 10); UA Fig 5 labels its lower end "to gain reduction control", so R5 is the series feed into the side-chain (the Peak Reduction / R37 network), and the attenuator branch (pin 7, R6, R7) is the other path. Whether R5's far end also has a ground reference on the TT drawing is not clear to me, (?).

### Optical attenuator (T4 / T4A)
UA Figure 5 (the input and gain-reduction circuit):
- Series path: transformer pin 7 -> **R6 68K** -> **R7 2.7K** -> **Gain R1 100K** (the Gain pot, whose wiper feeds the 12AX7 V1A grid).
- The photoconductive cell (CdS) is the **shunt leg**, drawn at the R7 / Gain-pot junction with its other end to ground. So the attenuator is a voltage divider: R6 + R7 = 70.7K on top, the cell on the bottom. High cell resistance = no loss; low cell resistance = loss.
- EL (electro-luminescent) panel: driven through **0.0047 uF** (printed ".0047 uF" in UA Fig 5, ".0047" inside the T4A outline on TT), from the 6AQ5 driver. TT shows **R8 22M** inside the dashed T4A outline next to the 0.0047 uF (function not stated; probably a bleeder, (?)).
- UA text: "the photo-cell acts as the bottom leg in a voltage divider"; cell resistance cannot reach zero so some signal always passes.
- A second, hand-matched photocell, lit by the same EL panel, is used for the meter (UA page 11 to 12).
- Derived (mine, not in any doc): 40 dB of reduction needs the cell at about 70.7K / 99 = 0.7K (ignoring the gain pot loading and the 2.7K/100K details).

Cell facts from the texts:
- EL panel light output follows the audio voltage directly, so "the response is instantaneous" and no rectifier or filter is needed (TT page 4). UA: EL panel was designed for 120 V 60 Hz night-lights; wide-band audio shortens its life.
- Cell is selected "to provide minimum attack time, and a release time which requires about 60 milliseconds for 50 percent release, and then a gradual release over a period of 1 to 15 seconds to the point of complete release" (TT page 4; note: the spec page says 0.5 to 5 s, see section 5).
- "The actual stage gains and tube operating parameters are not varied, permitting the tubes to operate at optimum conditions regardless of the amount of gain reduction" (TT page 3): the attenuator sits ahead of V1 and does not change any bias.

### Voltage amplifier V1 (12AX7A), schematic and UA Fig 7
TT schematic values; UA Fig 7 agrees except where noted:
- **V1A**: grid from the Gain pot wiper through **C1 .02 uF**. **R10 1.5K** cathode to ground. **R9 220K** plate load. Plate -> C1(.02) coupling -> V1B grid. (R9 appears at the top left of V1A in UA Fig 7 next to C1 .02 uF; the redraw confirms it is the plate load.)
- **R11 68K** runs from the V1A cathode node up to the supply node (both the TT scan and the redraw show it that way). Its function is not explained in the text (cathode lift or feedback return; (?)). Do not assume it is the feedback resistor.
- **V1B**: grid leak **R12 470K** to ground, **R13 220K** plate load, **R14 2.7K** cathode (the redraw prints 470K here, which is wrong against both TT and UA). Plate -> **C2 .1 uF** -> V2A grid.
- Supply decoupling for V1: **R16 68K** with **C7 30 uF / 450 V** (C7a..d are sections of one multi-section can in the TT drawing: C7a/C7b/C7c/C7d are labelled in different places; UA Fig 6 shows "C7D 30 uF" at the 6AQ5 screen and Fig 7 shows "C7C 30uF").
- TT text: "The voltage amplifier stage provides a gain of 40 dB. Overall amplifier feedback of approximately 20 dB provides low distortion, flat response, and gain stability."
- **Where the feedback is returned is not clearly legible** in any of the images. The block diagrams (TT Fig 2, UA Fig 3) show a "negative feedback" arrow from the cathode-follower output (before the output transformer) back to the V1 voltage amplifier. The only candidates in the drawing are R11 68K / R10 1.5K at the V1A cathode and the long wires that run to T2; I could not trace which one. (?)

### Output stage V2 (12BH7A): "totem pole" double cathode follower
- TT: "The output stage is somewhat unconventional in that a totem pole or double cathode follower is used. This output stage can tolerate great amounts of output impedance mismatch, but retains low distortion and flat frequency response." UA: 12AX7 voltage amplifier followed by a 12BH7A cathode follower, then the output transformer.
- **V2A**: grid from V1B plate through C2 .1 uF; **R15 470K** grid leak; **R17 10K** plate load (with **C3 .1 uF**, placement near the plate/grid, (?)); **R18 1K** cathode resistor, which ties the V2A cathode to the V2B plate.
- **V2B** (lower triode, the "active cathode load"): **R19 470K** to its grid, **R20 1K** cathode to ground.
- Output node (V2A cathode / V2B plate) -> **C5 10 uF / 450 V** -> output transformer T2 primary. **R21 100K** from that side to the supply rail (DC path for the output coupling cap; (?) which end is which).
- **R22 33K** with **C4 50 uF** (voltage printed "350 V" or "390", (?)) is the extra supply filter for the output stage.
- Output transformer: **UTC A-24**; TB2 terminals: 10 = 600, 9 = centre tap, 8 = 600, 7 = ground, 6 = stereo/parallel (the link terminal). Output impedance 50/150/250/600 balanced or unbalanced (TT), 600 balanced (UA); source impedance "approximately 150 ohm at 800 cps" (TT page 3 spec 12).

### Gain
- Spec gain **40 +/- 1 dB** (UA and TT). TT Figure 1 plot (gain reduction curve) has the "no gain reduction" line passing through (input -30 dBm, output 0 dBm), which is 30 dB of gain, not 40; the plot is "typical" and the Gain knob / padding setting is not stated.
- TT: "sufficient fixed pad should be placed ahead of the LA-2A to allow normal output at approximately 50% setting of the gain control", and to avoid overloading the input transformer.

## 2. Controls

- **Gain** (R1 100K, in series just before V1A): sets output level only; "does not affect the compression" (UA). The two controls are independent (TT, UA). TT labels it "Gain (input level)"; UA labels it "Gain (Output level)". Physically it is the pot between the attenuator and V1A grid, so it is a pre-amplifier-level control in the signal path but is after the side-chain tap, which is why it does not change the threshold.
- **Peak Reduction** (R2 **100K**, the grid control of V3A, the side-chain amplifier input): "controls the gain of the side-chain circuit. The greater the gain of this circuit, the lower the threshold and the greater the amount of compression" (UA). TT: "the point at which the compressed curve breaks away ... is determined by the setting of the Peak Reduction control". UA says the LA-2A "while also having a fixed threshold, does not control the input level, but rather controls the amount of side chain gain". So Peak Reduction sets both threshold and amount.
- **Meter selector** (3-position): 1 Output +10, 2 Gain Reduction, 3 Output +4 (TT schematic). The meter reads 0 VU at +10 dBm or +4 dBm output (TT page 5).
- **Compress / Limit** switch: see section 3.
- Rear controls: **R37 1M** (pot, "LIM RESP", gain-reduction frequency response, see below), **R3 1M** (stereo balance adjust, "STEREO ADJ").

### Rear side-chain frequency response (R37)
- Why: FM broadcast pre-emphasis adds 17 dB at 15 kHz (75 microsecond network); R37 lets the limiter react more to highs (TT page 6/7, UA calibration).
- R37 1M pot is in the side-chain feed, shunted by **C12 .001 uF** (UA Fig 6). "Increasing the resistance of R37 reduces the amplitude of the low frequency voltage applied to the Peak Reduction control, R2. The high frequency components are not affected because of the low reactance of C12." (TT.)
- Flat = fully clockwise (factory) (UA). Fully counter-clockwise gives "approximately 10 dB more reduction at 15 kHz than at frequencies below 1 kHz" (TT).
- Corner of the R37/C12 pair at the full 1M setting: 1/(2 pi 1M 1nF) = 159 Hz (derived).

## 3. Compress / Limit switch

- TT addendum (LA-2A (11-66)): "All LA-2A Leveling Amplifiers after serial number 572 incorporate a leveling slope control switch ... to allow selection of either limiter or compressor action. ... Beyond the break away point the output does not increase with an increase of input level, producing a plot which is horizontal [limiter]. When the switch is set for compressor operation, the output will increase slightly with increasing input level. The resulting curve will have a slope of approximately 3:1, which means that an increase of input level of 100% will cause an output increase of 30%." The switch is on the chassis adjacent to the optical attenuator (T4A) socket.
- UA page 3: "The Limit/Compress Switch changes the characteristics of the compressor IO curve. When in the Compress position, the curve is more gentle, and presents a low compression ratio. A higher compression ratio results when the switch is set to the Limit position." No numbers.
- **Exact component values the switch changes: not given anywhere in UA, UA = doc70f05, or TT.** The TT schematic on page 9 is the pre-switch drawing (the switch was added after serial 572) and has no switch on it. UA gives no schematic of it. Anything claimed about the resistor or capacitor it switches would be a guess.
- (Cross-check only, low trust.) The 2018 redraw shows a single-pole switch "SW3, LIMIT / COMP." whose COMP position ties the R6/R7 junction to the node after R7 (2.7K), i.e. it appears to short R7 2.7K. That cannot produce a 3:1 slope on its own (R7 is 4 percent of the 70.7K series arm), so I do not trust that drawing for this switch.

## 4. Side-chain amplifier and EL panel drive

UA Figure 6, TT schematic:
- Tap: from the input transformer secondary (through R5 68K) through the **R37 1M / C12 .001** network to the Peak Reduction pot **R2 100K** (with **R30 47K** and **C6 .01 uF** in a shunt branch at the R2 input, as printed, (?) exact connection).
- **V3 (12AX7A)** is the control (side-chain) voltage amplifier, a two-triode stage with **R32 1K** between the cathodes, **R31 1K** to ground (cathode), **C8 .03 uF** in the cathode network, **R33 220K** plate load, **C9 .02 uF** coupling. Output from V3 goes (a) to the **stereo parallel terminal (TB2 pin 6)** and (b) through **R3 1M** (stereo balance adjust pot) to the driver grid. TT: in stereo operation the control voltage is common to both units, so either channel's gain-reduction signal reduces both.
- **V4 6AQ5A** is the luminescent driver (an output pentode wired as the EL panel driver): **R35 220K** screen supply with **C7 30 uF / 450 V** decoupling (printed "C7D"), **R36 1K** cathode with **C10 50 uF / 25 V** bypass, **R34 22K, 2 W** plate load, and **C11 .1 uF** (printed ".01" on the TT scan, ".1" on UA Fig 6; (?)) coupling the plate to the EL panel (which is in series with the 0.0047 uF inside T4A).
- Rated driving: the 6AQ5 drives the EL element directly; EL panels are made for 120 V / 60 Hz, so audio operation is a stress on the panel (UA).
- Block diagram (UA Fig 3, TT Fig 2): input transformer -> [attenuator T4 -> gain pot -> V1 -> cathode follower V2 -> output transformer], and, in parallel, input -> [pre-emphasis / R37] -> Peak Reduction -> V3 -> R3 stereo balance -> V4 -> EL panel.
- **Feed-forward or feed-back?** UA (page 11): "The LA-2A is a feed-back style compressor. This is due to the fact that the signal that is used to drive the side-chain circuit is affected by the gain reduced signal." But the TT schematic and TT Figure 2 show the side-chain tapped from the input transformer, ahead of the attenuator, and TT text says "a portion of the input signal is fed through the gain reduction control to the 12AX7 control amplifier (V3)". The two docs disagree; the primary drawing looks feed-forward (detector sees the un-attenuated input). I could not find a wire from the amplifier side back to the side-chain input on the TT drawing.
- There is no rectifier or detector anywhere in the side-chain (TT page 4): the audio itself lights the EL panel, and the cell's own response is the detector, attack and release. UA: "the attack and release of the LA-2A are completely determined by the T4."

## 5. Meter circuit (TT schematic, UA page 11 to 12)

- Second cell **T4A (meter cell)** lit by the same EL panel, "hand-selected" to match the gain-reduction cell. It sits in a bridge: **R25 18K to 68K** ("value selected to match T4A for correct dB gain reduction reading", note 3 on the schematic), **R27 10K**, **R4 1M** (zero adjust pot, "ZERO ADJ"; UA: screw-adjust pot next to the Limit / Compress switch, set with the meter in gain-reduction mode and no signal), **R26 330K**, **R28 470K** dropping from the supply to an **NE-2 neon** (marked "65 V", the regulated bridge supply).
- Meter shunt / multiplier resistors for the +4 and +10 positions: **R24 3.9K** and **R23 6.8K** (read from the schematic; which one is on which position, (?)).
- Meter: "VU meter"; calibrated 0 VU = +10 dBm or +4 dBm, or dB of gain reduction.
- Meter zero set with no signal and the switch on Gain Reduction (UA page 4; TT page 5).

## 6. Power supply (TT schematic)

- Transformer T3: one high-voltage secondary labelled **"500 V CT 40 mA"** (read from the schematic, (?) for the exact wording), full-wave with **CR1 and CR2, 1N3196** diodes (the original is silicon).
- Filter: **R29 4.7K, 2 W** between two **40 uF / 450 V** sections ("40/450V" printed twice).
- B+ markings: the rail after the filter is marked about **275 V**; a lower node near V1/V2 reads **205 V** (printed beside R13, the V1B plate supply node after R16, (?)); the neon node reads **65 V**.
- Heaters and meter lamp: **6.3 V, 2 A** winding (printed "6.3 V to heaters and meter lamp"). Heater of the 6AQ5 and the 12AX7/12BH7 are all fed from it.
- Line: 115 V as shipped; for 230 V, remove jumpers 1-2 and 3-4 on the terminal strip and join 2 to 3 (TT page 7). Fuse values above.

## 7. Spec numbers, with the document each one comes from

Tags: UA = UA manual (also doc70f05); TT = Teletronix original; TT-add = Teletronix 11-66 addendum.

| Item | Value | Source |
|---|---|---|
| Gain reduction, maximum | up to 40 dB | UA, TT |
| Gain reduction, instantaneous | "up to 40 DB of instantaneous gain reduction, yet causes no wave form or harmonic distortion" | TT intro |
| THD at +10 dBm output | less than 0.35 % | UA, TT |
| THD at +16 dBm output | less than 0.75 % | UA, TT |
| THD, 30 dB of gain reduction | "at least 30 DB of gain reduction or limiting with less than 0.5 % harmonic distortion" | TT (gain-reduction frequency response section) |
| Frequency response | +/-0.1 dB, 30 Hz to 15 kHz ("30 cycles to 15 KC" in TT) | UA, TT; the UA appendix repeats "0.1 dB from 30 to 15,000 Hz" |
| Noise | 75 dB below +10 dBm output | UA, TT specs |
| Noise (appendix) | "better than 70 dB below plus 10 dBm output" | UA appendix, page 12 (different from the spec page) |
| Gain | 40 +/- 1 dB | UA, TT |
| Output level | +10 dBm nominal, +16 dBm peaks | UA, TT |
| Input level | +16 dBm maximum | UA, TT |
| Input impedance | 600 ohm balanced (UA); 50/150/250/600 ohm, balanced or unbalanced (TT) | UA, TT |
| Output impedance | 600 ohm balanced (UA); 50/150/250/600 ohm (TT) | UA, TT |
| Output source impedance | approximately 150 ohm at 800 cps | TT |
| Attack time | "Very fast attack time" (no number) | UA |
| Attack time | "essentially instantaneous (10 usec)" (the OCR reads "10 usee"; the page image reads 10 microseconds) | TT spec 8 |
| Attack (cell) | "A cell is selected which provides minimum attack time" | TT page 4 |
| Release, first stage | 0.06 s for 50 % release | UA spec, TT spec 9 |
| Release, complete | 0.5 to 5 s depending on the amount of previous reduction | UA spec, TT spec 9 |
| Release, complete (circuit description) | gradual release over 1 to 15 s to the point of complete release | TT page 4 (disagrees with its own spec page) |
| Two-stage decay of the CdS cell | after the light is removed "releases quickly (40-80 milliseconds) to approximately half of its off resistance. The remainder of its release can take place over as much as several seconds" | UA page 10 |
| Memory | release is slower "if the unit has either been in compression for a while, or the amount of compression is large"; signal-dependent release | UA page 10 |
| Compression curve, breakaway | breaks away from the straight line at about -30 dBm input (gain-reduction plot, TT Fig 1) | TT page 3 and Fig 1 |
| Compression region | "Compressor action occurs from the breakaway point at -30 DB input and up to -20 DB, at which point the curve becomes horizontal to exhibit limiting action. The input increases an additional 20 DB, but the output increases less than 1 DB." | TT page 3 |
| Plot read-off | out about 0 dBm at -30 in; about +4.5 at -25; plateau about +5.3 to +5.5 dBm from -20 to 0 dBm input (read by eye from Figure 1) | TT Fig 1 (my reading) |
| Compression "gradually increases over the first 10 DB of input level rise", then the slope becomes horizontal | | TT page 5 |
| Ratio, Limit position | horizontal (infinite) after the knee | TT page 3/5, TT-add |
| Ratio, Compress position | about 3:1 ("an increase of input level of 100 % will cause an output increase of 30 %") | TT-add |
| Ratio, qualitative | Compress = gentle, low ratio; Limit = higher ratio | UA |
| Threshold | Peak Reduction control sets both threshold and the amount; threshold itself is fixed, the side-chain gain moves | UA page 10, TT |
| Recommended use | 4 to 8 dB of compression "continuously"; continuous 20 to 30 dB "does tend to reduce the dynamic range of music" | TT page 5 |
| Pre-emphasis range | R37: about 10 dB more reduction at 15 kHz than below 1 kHz at its maximum HF setting | TT page 7 |
| FM pre-emphasis context | 17 dB at 15 kHz (75 microsecond network) | UA, TT |
| Stereo link | wire <2 ft, shielded, terminals 6 and 7 of each unit; set R3 so both meters show the same reduction at about 5 dB | UA, TT |
| Overall feedback | approximately 20 dB | TT page 4 |
| V1 stage gain | 40 dB | TT page 4 |
| Power | 115/230 V, 50 to 60 Hz, 35 W | TT |
| Size | 19 in x 5 1/4 in panel, 7 1/4 in behind panel | UA, TT |
| Tubes | (2) 12AX7A, (1) 12BH7A, (1) 6AQ5 | UA, TT |

### Things the three documents do NOT give
- Any numeric THD for the Compress vs Limit switch positions.
- Any release time that depends on the switch.
- The component values the Compress/Limit switch changes.
- Knee width in dB (only "gradually over the first 10 DB").
- Attack time of the photocell itself in numbers (only the EL panel "instantaneous / 10 usec").
- The path of the overall feedback.

## 8. Redraw cross-check (extra file, low trust)

The redraw (repo `reference/compressors/sources/Teletronix_LA-2A.pdf`) agrees with TT on: R6 68K, R7 2K7, R5 68K, Gain 100K, R10 1K5, R9 220K, R11 68K, R12 470K, R13 220K, R15 470K, R17 10K, R18 1K, R19 470K, R20 1K, R21 100K, R22 33K, C5 10 uF, R33 220K, R37 1M, C12 1n, R3 1M, R2 100K, R30 47K, C6 10n, R31 1K, R32 1K, R4 1M, R27 10K, R26 330K, R28 470K, R23 6k8, R24 3k9, R25 27k to 100k, NE-2.
It disagrees with the originals on: **R14 470K** (TT and UA: 2.7K), **R34 10K** (TT: 22K, 2 W), an extra **R38 220K / C13 510p** at the 6AQ5, **C1 / C2 / C3** values given in nF (20n / 100n / 100n, which match .02 / .1 / .1), a **4k7** R29, and 1N4007 diodes with a 300 V 40 mA winding (TT says 500 V CT 40 mA with 1N3196). It also shows a variable cap (C4 30-3800) and a 50-380 trimmer cap in the side-chain, and a 230 V transformer, which are not in the original. Use it for topology hints only.

## 9. Summary of the circuit in one paragraph

Input transformer (UTC HA-100X) -> R6 68K + R7 2.7K series arm, with the T4 CdS cell as the shunt leg (the attenuator) -> Gain pot R1 100K -> V1 12AX7A (V1A: R10 1.5K cathode, 220K plate; V1B: 2.7K cathode, 220K plate; about 40 dB, with about 20 dB of overall negative feedback) -> C2 .1 -> V2 12BH7A double (totem-pole) cathode follower (R17 10K plate, R18 1K, R20 1K, R19/R15 470K) -> C5 10 uF -> UTC A-24 output transformer (600 ohm balanced). In parallel a side-chain takes the input transformer's signal through the R37 1M / C12 .001 response network and the Peak Reduction pot R2 100K to V3 12AX7A (220K plate load, 1K cathode resistors), then through the stereo balance pot R3 1M to V4 6AQ5A (R34 22K 2W plate load, R36 1K cathode, R35 220K screen), which drives the EL panel (through 0.0047 uF) that lights the CdS cell. There is no rectifier or time-constant network anywhere, so attack and release are the properties of the EL panel and the cell. Power supply: 500 V CT 40 mA winding, two 1N3196 diodes, 4.7K 2 W, two 40 uF / 450 V, B+ about 275 V (and 205 V after R16), a neon NE-2 at 65 V for the meter bridge, 6.3 V heater supply. The Compress/Limit switch (not in the 1966 original drawing; added after serial number 572) changes the curve from horizontal (limit) to about 3:1 (compress); its parts are not shown in these documents.
