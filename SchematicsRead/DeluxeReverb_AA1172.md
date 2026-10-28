# Fender Deluxe Reverb-Amp AA1172 (silverface, early) - schematic read

Source files: `39ed9f59-Fender-Deluxe-Reverb-A1172-Schematic.pdf` and `5048e5fc-Fender-Deluxe-Reverb-A1172-Schematic_1.pdf`. **Byte-identical** (md5 b4e56a5e...), one page each, a single embedded 360 dpi grey scan (rotated 90 deg in the PDF; I rotated it for reading). Clean, high-contrast scan; most values very legible.
Title: "FENDER MODEL DELUXE REVERB-AMP A1172 SCHEMATIC NOTICE" (the file name and the sheet call it "A1172"; Fender code AA1172). Title block: "SCHEMATIC & WIRING DIAGRAM DELUXE REV-AMP", **Dwg No. 045450, Rev H**, size C, rel. no. 530. Footer: "Fender Musical Instruments, CBS Musical Instruments, a division of Columbia Broadcasting Systems Inc., Fullerton, California, U.S.A."
Notes on sheet: voltages read to ground with electronic voltmeter, values **+/-10 %** (AB763: +/-20 %); resistors 1/2 W 10 % unless specified; "all capacitors at least **600** volt rating if not specified" (AB763/AB868: 400 V).
Convention: "1500" = 1.5k, "6800" = 6.8k, "2700", "2200", "820" in ohms; caps in uF unless "PF". Pot suffix "-J" appears on the tone-stack pots (250K-J, 1M-J); I read it as a Fender pot-taper code (the AB763 shows "A"/"2-35"); I do not know what taper "J" denotes on this sheet (?).

## 1. Tube complement and heaters
| Ref | Tube | Function |
|---|---|---|
| V1 | 7025 | Normal channel, V1A + V1B |
| V2 | 7025 | Vibrato channel, V2A + V2B |
| V3 | 12AT7 | Reverb driver (halves paralleled) |
| V4 | 7025 | V4A = mixer stage, V4B = reverb recovery |
| V5 | 12AX7A | Tremolo oscillator (V5A) + modulator (V5B) |
| V6 | 12AT7 | Phase inverter (long-tailed pair) |
| V7, V8 | 6V6GT | Push-pull output |
| V9 | 5U4GB | Rectifier |
- Heaters: 6.3 V winding with **two 100 ohm resistors** from the two winding ends to ground (artificial centre tap); "TO ALL 6.3 V HEATERS AND PILOT LIGHT".
- Mains 120 V 60 Hz; ground switch + **.047 uF 600 V**; AC switch; **2 AMP SLO BLO fuse**.

## 2. Stage by stage
### Normal channel (V1)
- Input: jacks 1/2, each 68k into the common V1A grid node; **1M** from jack-1 node to ground. (Jack 2 = pad input.)
- V1A: plate **+180 V**, plate load **100k 1 W** (to node on the shared +B line); cathode **1500 ohm || 25 uF**, **+1.3 V**.
- Tone stack (section 3). Volume **1M-J**; no bright cap.
- V1B: plate **+180 V**, plate load **100k 1 W**; cathode node A: **820 ohm || 25 uF** shared with V2B, **+1.3 V**.
- Output: **.047 uF -> 220k** -> node M (V4A plate node; M also couples to PI).
### Vibrato channel (V2)
- Input as Normal.
- V2A: plate **+170 V**, plate load 100k 1 W, cathode 1500 ohm || 25 uF, +1.3 V.
- Tone stack as Normal. Volume **1M-J** with **47 pF bright cap** across the top half.
- V2B: plate **+170 V**, plate load **100k 1 W**; cathode **820 ohm || 25 uF** (node A), +1.3 V.
- Output **.022 uF** (printed ".022") -> V4A grid node. (AB763/AB868: .02.)
- Preamp B+ feed: the V1A and V2A plate loads hang on a **100k 1 W** resistor chain from the supply (printed "100K,1W" twice on the vertical line between the stages). I read: each stage has its own plate resistor, with additional "100K, 1W" dropper(s) in the supply line to the first stages (?). The same wording is on AB868.

## 3. Tone stack (both channels)
- Plate node -> **250 pF** -> treble pot top (**TREBLE 250K-J**).
- Plate node -> **100k** -> stack node; stack node -> **.1 uF** -> bass pot top (= treble bottom); stack node -> **.047 uF** -> bass pot bottom; bass bottom -> **6800 ohm** -> ground.
- **BASS 250K-J** (wired as variable resistor), **VOLUME 1M-J**.
- Bright: 47 pF on Vibrato channel only.

## 4. Reverb
- **V3 12AT7 driver**, halves paralleled; grid via **500 pF** from the vibrato-channel output/mixer-grid node; grid leak **1M**; cathodes **2200 ohm || 25 uF**, **+8.7 V**; plates **+410 V** into **TR4** primary (other end to node B). TR4 (export table: 125A20B). Secondary to the pan input.
- **Recovery (V4B, lower 1/2 7025)**: grid from pan output with **220k to ground, with 2000 pF across it** (new vs AB763/AB868); cathode **820 ohm || 25 uF**, **+1.3 V** (node E); plate **+170 V**, plate load **100k 1 W** to node D; output **.003 uF** -> **REVERB 100K-L** -> wiper -> **470k** to the mixer-grid node; **220k** from that node to ground. Footswitch shorts recovery grid.
- **Mixer (V4A, upper 1/2 7025)**: grid node gets V2B via .022 uF, and also (**3.3M || 10 pF**) from the reverb mix node; plate **+180 V**; cathode node E (+1.3 V, shared with recovery). V4A plate node M: also takes V1B via .047/220k; M -> .01 uF -> PI grid. From M, **220k** goes down to a node that has a **.1 uF**; the 100k 1 W to node D is the plate supply. The exact placement of the .1 uF and 220k relative to the 100k 1 W supply (series decoupling filter vs plate load) is (?) - see section 9.
- Relative to AB763/868 the V4A grid network and the 500 pF/1M driver are the same.

## 5. Tremolo (V5 12AX7A) - this scan is clear
- **V5A oscillator**: plate **+270 V**, plate load **220k**; cathode **2700 ohm || 25 uF**, **+2.1 V**.
- **V5B modulator**: plate **+345 V**; cathode **100k || 5 uF 50 V** (printed "5 50"; AB763 appears "25"), cathode **+17.0 V**; **10M** from the plate node to the B node; neon lamp with its own **100k** to the B node; the photo-resistor element of the opto (drawn inside the same circle) connects to the **50K-RA INTENSITY** pot.
- Phase-shift network: **1M, 1M** series arms, **2.2M** from the pedal-jack node to ground, **.01** (to ground at the 1M-1M junction side), **.01** series, **.022** from the V5A plate side to the network, **3M-RA SPEED** pot in series with **100k** to ground (SPEED sets the first leg). VIBRATO pedal jack at the input node of the network.
- 10k + the 10K-L pot are the **bias** components (section 7), not tremolo.

## 6. Phase inverter V6 12AT7 (long-tailed pair)
- V6A grid: **.01 uF** from node M (AB763: .001); **330k** from grid to the bias node (+75.5 V) (AB763: 1M).
- V6B grid: **.1 uF - 200 V** from the NFB node; **330k** to the bias node.
- Common cathodes **+90 V** via **470 ohm** from the +75.5 V bias node; bias node via **22k** to the NFB node; NFB node via **47 ohm** to ground.
- Plate loads: **82k** (V6A) and **100k** (V6B), no "5 %" marking on this sheet; both from node **C** printed **+375 V**; plate voltages **+190 V** (V6A), **+180 V** (V6B).
- **NFB: 820 ohm** from output-transformer secondary (speaker line, above the EXT SPKR jack) to the NFB node. Feedback ratio 47/(820+47) = 5.4 % (-25.3 dB).
- Coupling: **.1 uF** to each 6V6 grid.

## 7. Output stage
- 2 x 6V6GT, fixed bias, cathodes grounded.
- Plates **+415 V**; screens through **470 ohm 1 W** each from the **+415 V** screen line; OT centre tap **+420 V**.
- Grid stoppers **1500 ohm**; grid-leak **220k** each; **1200 pF** from each 6V6 grid (after the 1500) to ground (two caps labelled "1200PF").
- **Bias**: **-35 V**. Chain: 330 VAC lead -> **470 ohm 1 W** -> diode (marked "-" at the pot node, "+" at the 470 side) -> **50 uF / 70 V** (printed 50 / 70) -> **10K-L** bias pot (with **10k** from the far end to ground), wiper to the bias line.
- OT **TR3** (FMI P/N 022640). Speaker **8 ohm** printed ("8 OHM SPKR"); EXT SPKR jack. OT primary Z not printed.
- NFB: 820 ohm from secondary (see 6).

## 8. Supply
- 5U4GB, **330 VAC** each side of the HV winding.
- Rectifier cathode -> STANDBY switch -> **two 16 uF 450 V** (in parallel) -> choke **TR2** (P/N 022707) -> node **B** **16 uF 450 V** -> **2.2k 1 W** -> node **C** **16 uF 450 V** -> **10k 1 W** -> node **D** **16 uF 450 V**.
- Printed: **+420 V** (OT CT / TR3 supply tap), **+415 V** (plates/screens), **+410 V** V3 plate (node B), **+375 V** (node C, PI plates), **+345 V** V5B plate, **+270 V** V5A plate, preamp plates +170 / +180 V, V4B plate +170 V.
- Bias -35 V.
- Transformer table (export): TR1-125P33A (022749), TR1-125P23B (025130), TR2-125C3A (022707), TR3-125A1A (022640), TR4-125A20B (022921).

## 9. Not legible / uncertain
- Pot taper letter "J" on 250K / 1M pots.
- Whether the preamp 100K 1 W resistors are the stage plate loads only or also include a dropper in the supply line.
- Exact connectivity of V4A plate node / 220k / .1 uF / 100k 1 W (series filter vs plate load).
- Voltage at nodes D (not printed).
- The two electrolytics at the reservoir are drawn in parallel as printed; "16" and "450" values legible.
