# ENGL Powerball (schematic 645-1, two sheets: 1/2 audio, 2/2 relay and sense logic)

Source images: `powerball-1.png` (1938x1375, sheet 1/2, readable at crop level) and `powerball-2.png` (sheet 2/2, small; relay and LED logic only). Resistors printed with "*" mean a value that is a tolerance or specially selected part (the printed star is part of the label). Values marked (?) are not reliable. Computed values use my own nodal solver of the plugin's topology with source and load assumed, and are estimates.

## 1. Tube complement

| Ref | Type | Stage |
|---|---|---|
| V5A | ECC83 | Input stage (both channels' preamp, gain pots) |
| V5B | ECC83 | Second stage (clean/crunch and lead path) |
| V6A | ECC83 | Third stage (gain stage for clean/crunch/lead; relay selected) |
| V6B | ECC83 | Fourth stage (channel 2 / lead path) |
| V7A | ECC83 | Stage ahead of FX send (driver) |
| V7B | ECC83 | Stage before the Master A/B pots |
| V8A, V8B | ECC83 | Phase inverter (dual-triode long-tail with separate plate/cathode networks) |
| V1 to V4 | 6L6GC | Four-tube push-pull power amp |
Six ECC83 stages in front of the PI plus two (V8A/B) as PI. Heaters: H-V1 to H-V4 and H-V8 on 6.3 V AC; H-V5 to H-V7 on rectified DC (DI7 to DI10 1N4007 bridge, CE15/CE16 6800 uF).

## 2. Preamp stage by stage (sheet 1)

Supply label "450 V" at the top rail, CE3 10 uF filter.

### V5A (input)
- Plate load R3 100K* (1 W), plate coupling C1 47 nF/630 V; cathode R2 1K parallel CE1 22 uF (corner about 7 Hz; 1/(2 pi 1K 22 uF)); grid leak R1 1M; input jack BU1 (grid stopper not drawn).
- Plate network after C1: R4 2.2M to ground; C2 15 nF with the "Bottom" switch (S2A) and a 100 pF bright cap C3 with the "Bright" switch (S1A) around the gain pots.
- Channel 1 gain P1 1 MA (1M audio taper) with R5 100K in series at the top; Channel 2 gain P10 1MA with R6 100K. Channel select through lamp-LDRs LD1 and LD2 (LT30), R7 1M.

### V5B
- Plate R9 100K*, cathode R8 1.5K parallel CE2 22 uF (about 5 Hz); coupling C4 47 nF into the divider chain R10 2.2M, R11 220K / R12 470K, R13 100K / R14 470K, plus R15 1M, R18 680K, R16 330K, R17 220K. These are the "Gain" relay (RE3A / RE3B) range dividers: relay-selected attenuators that set the gain structure between clean/crunch and lead.

### V6A
- Plate R20 330K* (1 W), cathode R19 3.3K parallel CE4 1 uF (about 48 Hz); coupling C5 47 nF through relays (RE1B / RE2A / RE2B) to the channel stacks.

### V6B
- Plate R24 100K*, cathode R23 1.5K parallel CE5 22 uF; grid R21 220K / R22 1M; C6 100 nF coupling; R25 1M.

### V7A and V7B
- V7A: plate R43 100K* with C17 10 nF/630 V across it (high-frequency shelf on the plate), cathode R42 1K parallel CE6 4.7 uF, grids R40 220K and R41 100K; this stage feeds the FX send BU6 via a J175 JFET mute (T1, T8) driven by relay logic. Return BU7 comes back through P19A/P19B balance (50KB x2) with R45/R46 10K and R44 47K.
- V7B: plate R49 100K*, C18 100 nF into MASTER A (P17 1MB) / MASTER B (P18 1MB) through R50/R51 100K, selectable by RE9A; C19 100 nF into R52 1M and the PI. Cathode R48 1.5K parallel CE7 22 uF; grid R47 100K.

## 3. Tone stacks (two, relay switched; sheet 1 lower left)

### Clean / Crunch stack
- Slope resistor R27 100K.
- Treble: two caps C7 470 pF and C8 470 pF ("Style" relay RE6A puts C7 in parallel with C8, giving 940 pF); pots P4 "Clean treble" 250KB and P5 "Crunch treble" 250KB (selected by RE5A).
- Bass: P2 1MA with C9 220 nF; C10 100 nF and C11 47 nF are the style-selected mid caps (RE6B); Middle P3 20KB.
- Outputs: P6 "Clean volume" 250KB (with R28 220K) and P7 "Crunch volume" 250KA (with R29 330K), each through the Gain relay RE4A.
- Computed (source about 40K, load about 300K, both assumed; bass audio taper, treble/mid linear-position): Style off (470 pF, 220 nF, 47 nF): noon insertion loss mean about -16.8 dB, -15.3 dB at 1 kHz, notch about -23 dB near 280 Hz, all-10 minimum -22 dB near 190 Hz. Style on (940 pF): mean about -14.8 dB, -12.3 dB at 1 kHz, notch about -21 dB near 200 Hz.

### Lead stack
- Slope resistor R31 100K; treble caps C12 470 pF and C13 470 pF (Style relay RE7A for the 940 pF state); Lead Treble P14 250KB; C14 220 nF; Lead Bass P11 1MA; mid caps C15 47 nF and C16 22 nF through RE8A/RE8B with two middle pots P12 "open middle" 20KB and P13 "focused middle" 20KB (Style chooses which).
- Level: P15 "Lo Lead Volume" 250KB and P16 "Hi Lead Volume" 250KB; R32 330K feeding S3A (L.L.B., "lo lead boost") and S4A (H.L.B., "hi lead boost") with R33 470K / R34 680K / R35 470K / R36 680K dividers.
- Computed (same assumptions): open mid (470 pF, 220 nF, 22 nF): noon mean about -15.5 dB, -15.7 dB at 1 kHz, notch about -21 dB at 410 Hz; with Style (940 pF): mean about -13.7 dB, -12.4 dB at 1 kHz, notch about -18 dB near 290 Hz.

## 4. Channel switching and gain structure
Four modes (LEDs on sheet 2): Clean, Crunch, Lo Lead, Hi Lead. Relays RE1 to RE9 (and the lamp LDRs LD1, LD2) switch channels, gains, styles and master A/B; the logic is IC1 (LM324 quad comparator/driver for the mode LEDs on sheet 2) with transistors T2 to T6 (BC547, BC307B) driving the relay coils, 24 V from a 7824 regulator. Footswitch 1: Clean/Lead and Gain Lo/Hi; Footswitch 2: Contour and Master A/B ("Style" S5A/S5B on the panel); ext. connector Z-5 (BU4). Clean and Crunch share V5A, V5B, V6A with clean/crunch treble and volume pots; Lead uses V6A/V6B/V7 with the second stack. No clipper diodes or cold clippers are visible on the audio path; all clipping is tube saturation and the relay-switched gain dividers set the headroom.

## 5. Phase inverter and power amp
- PI: V8A and V8B ECC83 ("long-tail" with cathodes tied: R55 470R shared cathode, R56 22K, R57 4.7K); plates R61 82K* (1 W) and R62 100K* (1 W); grid of V8A R53 100K; coupling C20 and C21 47 nF*/630 V from the plates, and C22, C23 47 nF*/630 V as additional couplings to the four 330K grid leaks R63 to R66 (shown as "338K" in the picture, read as 330K).
- Output tubes: V1, V2, V3, V4 = 6L6GC. Grid stoppers R67, R69, R70, R71 2.2K; screen resistors R72 to R75 470R* 1.4 W; cathode current-sense resistors R131 to R134 4.7R with ZD4 to ZD7 ZY150 zeners in the relay-switched cathode path (RE12 to RE15 contacts, used for per-tube sensing).
- Bias: -55 V rail through DI5 (4007) / R139 10K / CE11 100 uF/63 V, trimmer TR1 25K with R140 22K, CE13 10 uF/63 V. Note: "Bias adjustment between 28 and 34 mA each power tube."
- Output transformer: taps 16 ohm, 8 ohm, 4 ohm (BU12 16 ohm, BU11 and BU10 8 ohm, BU9 and BU8 4 ohm sockets); BY509 diodes DI11/DI12 clamp from the primary ends.
- NFB and presence: feedback network from the OT: R58 1.5K, C27 10 nF in series with P9 "Depth" 1MA, R60 22K, P8 "Presence" 50KC, C25 100 nF, R59 150K into the PI cathode node (R56 22K / R57 4.7K to ground). First-order estimate of the loop attenuation at the injection node: 22K / (22K + 150K + the series presence/depth arms) = at best about -18 dB, falling with presence turned down. Which pot arm is shunted in which direction, and the exact tap used, is (?).

## 6. Supply
- B+: stand-by off 460 V, on (play) 440 V (printed "Stand By Off: 460 Volts / Stand By On: 440 Volts"); rail labelled +430 V after R136 470R* 1.4 W; reservoir CE8 220 uF/500 V, CE9 47 uF/500 V, rectifier four BYW56 (B56) diodes with 4.7 nF caps C31 to C35 across each, R135 330K* bleeder. Preamp supply: R137 2.2K* / R138 2.2K* (1.1 W each) with CE10 22 uF and three ZY150 zeners in series (about 450 V clamp, "450 V" printed) feeding the preamp rail.
- Low voltage: +24 V from 7824 (IC5) with CE12 470 uF/40 V, C36 100 nF, CE14 1 uF; -55 V bias; 6.3 V heaters (four 6.3 V lamps GB1 to GB4).
- Mains: 230 V (EU), 120 V (USA), 100 V (Japan), fuses 2.0 A / 2.5 A (230 V) and 4 A / 5 A (120 V), external and internal.

## 7. Tube-health sensing (sheet 2)
Four sense circuits per tube (V1 to V4): relay RE16A/B and RE17A/B select "Sense1/Sense2", 470K input resistors R110, R122, R111, R123, 22K, 10K, 100K resistors, ZPD4.7 zener diodes, CE22 to CE29 1 uF and 10 uF caps, LM324 quads IC2 and IC3 with LD7 to LD10 LEDs, relays RE13S (V1 to V4) and standby switch S8B/T7 BC547, R108 4.7K. This is a tube-failure/bias monitor and has no audio effect.

## 8. Illegible or uncertain
- Sheet 2 is small: the transistor and LM324 pin assignments and several resistor values (R86, R87, R90 to R100) are not reliable; the logic, not the audio, is represented.
- Exact node connections of the Gain relay dividers (R15 to R18) and where each relay contact selects (RE1A, RE3A/B, RE4A/B) are only partly resolved; exact feedback arm directions; the power amp B+ tap labels C/B/A on the OT.
