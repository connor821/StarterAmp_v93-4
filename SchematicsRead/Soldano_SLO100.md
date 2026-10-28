# Soldano Super Lead Overdrive SLO-100 and SLO-50 (hand-drawn signal-path schematic) - schematic read

- Source: two images of the same drawing sent in chat on 2026-10-28 (the page `robrobinette.com/How_the_Saldano_SLO-100_Works.htm`), 2000 px wide, no file names kept. Footers: "Drawn by RobRobinette.com  V1.6h" (first image, with the green clean path and red OD path coloured in) and "V1.6k" (second image, uncoloured). I could not tell the two revisions apart at this resolution: every value below reads the same on both. If a value differs in your copy of the page, the page wins.
- This is a signal-path drawing by a third party (not a factory sheet). Title printed: "Soldano Super Lead Overdrive SLO-100 & 50", "Switches Shown In OD Mode, Crunch/Clean in Clean, Bright Switch Off, Standby Closed".
- An earlier reading of the same drawing was applied in v93 (CIRCUITS.md, MODELED_GEAR.md "Tweed, SLO-100 and 1959 schematics") but the sheet itself was never filed; `Marshall_revisions.md` section 5.4 notes that no Soldano sheet was among the files. This file is that sheet's reading. Values marked (?) are not certain at this resolution.

## 1. Printed notes (bottom left of the drawing)
- Overdrive mode = OD switch open, or footswitch open = LDR2 and LDR3 on. Clean mode = switch closed, or footswitch closed = LDR1 and LDR4 on.
- The light-dependent resistors are Vactec VTL5C1 (LED plus photoresistor optoisolators), used as silent switches; LED on = switch on / closed.
- The Clean channel is active and in parallel with the OD channel in OD mode. The OD channel is deactivated in Clean mode.
- In OD mode the two channels are out of phase with no channel mixing resistors, for maximum interaction.
- The Crunch/Clean switch swaps two attenuating voltage dividers (the inset at the top left).
- FX send attenuator dumps 99 % of the guitar signal; the FX recovery brings about 80 % back.
- Bass frequencies are attenuated by partial cathode bypass, not by coupling-cap size.
- For the high-voltage bias tap insert a 220k 1 W resistor before the bias diode. All resistors are metal film 1/2 W 1 % unless noted (the author recommends 1 or 2 W metal film for less hiss). All caps .002 uF and smaller are dipped ceramic.
- Signal paths as printed. Clean: Preamp 1 + Crunch/Clean attenuator + Clean gain pot + Clean preamp 2 + attenuator + Clean preamp 3 + FX buffer + attenuator + FX jacks + FX recovery + tone stack buffer + tone stack + Clean master volume + LTP + power tubes + output transformer + speaker. Overdrive: Preamp 1 + (in parallel: Crunch/Clean attenuator + Clean gain pot + Clean preamp 2 + attenuator, and OD attenuator + OD gain pot + OD preamp 2 + attenuator + OD preamp 3 / cold clipper) + OD preamp 4 + FX buffer + attenuator + FX jacks + FX recovery + tone stack buffer + tone stack + OD master volume + LTP + power tubes + OT + speaker.

## 2. Preamp
### Preamp 1 ("Clean & OD Preamp 1", V1B, 1/2 12AX7)
- Input jack, 68k grid stop, 1M input resistor to ground. Cathode 1.8k with 1 uF bypass. Plate load 220k.
- Plate -> 0.02 uF coupling -> a "Treble Peaker": 0.002 uF in parallel with a 470k attenuator (dumps 48 %) -> the node that feeds both channels.

### Clean channel
- Crunch/Clean DPDT switch selects one of two attenuating dividers (inset, top left): Crunch = a 470k attenuator into the 500k log "Normal Gain" pot (output 48 % cut at maximum volume); Clean = 470k + 470k inserted by a 39k shunt, which dumps 96 % of the signal (output 96 % cut at maximum volume) (?).
- Bright switch with a 470 pF treble peaker across the attenuator (470k "dumps 48 % in Crunch" label) (?).
- Clean Preamp 2 (V1A, 1/2 12AX7): cathode 2.2k (not bypassed), plate load 100k. 0.02 uF coupling, then a 2.2M attenuator (dumps 87 %) with a 120 pF treble peaker, 330k grid leak on the next stage (?).
- Clean preamp 3 is the shared stage below (V3B).

### OD channel
- OD attenuator 470k (dumps 48 %) -> 0.001 uF -> OD gain pot 500k log -> 470k grid stop -> OD Preamp 2 (V2A): cathode 1.8k / 1 uF, plate load 100k.
- 0.02 uF coupling -> 470k attenuator, 1M to ground (dumps 32 %) -> OD Preamp 3 "Cold Clipper" (V2B, 1/2 12AX7): **cathode 39k, not bypassed**, plate load 100k; 0.001 uF "plate load bypass / high-cut filter" (?). LDR1 (closed = kills the OD signal) and the OD-open LDR sit at this stage.
- 2.2M / 2.2M "anti-pop" resistors and a 0.02 uF coupling join the two channels' outputs into the shared stage.

### OD Preamp 4 ("Primary Distortion Stage", V3B, 1/2 12AX7)
- 220k grid stop, cathode 1.8k / 1 uF, plate load 220k. In Clean mode this is the Clean preamp 3.

### FX buffer, FX loop and recovery
- FX buffer: V3A cathode follower (plate load 220k, cathode 100k), then a 1k + "RL" attenuator that dumps 99 %; 1 uF coupling to the FX send jack ("Resistive load 2.2k was original value; FX loop too hot" note).
- FX return jack -> 1M grid leak -> FX recovery (V4B): plate load 220k, cathode 2.2k with 25 uF bypass ("bypass added by production change; 1 uF also used").

### Tone stack buffer and tone stack (late in the circuit)
- Tone stack buffer: V4A cathode follower, 100k cathode.
- Stack: 470 pF from the stack input to the top of the 250k linear treble pot; 47k "tone slope" resistor; 0.02 uF from the slope node to the treble pot bottom; 1M audio bass (wired as a rheostat); 25k linear mid pot with its own 0.02 uF back to the slope node ("Late in circuit tone stack"; "Mid in circuit" switch contacts (?)).
- Pre-phase-inverter master volumes: two 1M audio pots, "OD MV" and "Normal MV", selected by LDR3 / LDR4 ("MV select LDR switches"), after a 0.02 uF coupling.

## 3. Power amp
- Long-tailed phase inverter V5 (12AX7): 1M grid leaks (PI tail), 10k and 470 ohm cathode / tail resistors, 82k and 100k plate loads, 47 pF snubber between the plates, 0.047 uF coupling caps.
- Power tubes: two 6L6GC (V6, V7), plates 495 V, 2.2k grid stops, 220k grid leaks, 470 ohm 1 W screen resistors, fixed bias (bias pot 25k, 56k and 15k, 10 uF / 100 V caps, a 1N4007 on the bias tap).
- Negative feedback: **39k** from the 4 ohm tap, "2.6 times more NFB than the JCM800, from the 4 ohm tap"; Presence 25k log with 0.1 uF, 4.7k "NFB tail"; an optional Depth control (1M with 0.1 uF and 0.0047 uF) can be added.
- Output transformer: primary 4.4k ohm (50 W version) or 2.2k ohm (100 W version); taps 4, 8, 16 ohm with an Impedance switch.

## 4. Supply
- Power transformer, 100 W: 360-0-50-360 V 400 mA, heaters 6.3 V 9 A, 6.3 V switching winding 2 A. 50 W: same secondary at 200 mA, heaters 6 A, switching 2 A. Fuses: mains 3 A (50 W) / 5 A (100 W) slow blow; B+ fuse 0.5 A (50 W) / 1 A (100 W).
- Bridge of four diodes, standby switch (SPST), 2 x 200 uF / 300 V (= 200 uF / 600 V) reservoirs with 220k bleeders, choke 10 H / 200 mA.
- Nodes as printed: B+1 497 V (plates), B+2 495 V (screens, 200 uF / 300 V pair), B+3 378 V through 10k 3 W (40 uF / 450 V filter) to the PI and the tone-stack buffer, B+4 350 V through 15k (10 uF / 450 V) to the preamp and FX loop, B+5 359 V through 15k (10 uF / 450 V).
- Light-switching supply: a separate 6.3 V winding, bridge rectifier DB101, 1000 uF / 16 V, dropping resistors 15 ohm, 33 ohm, 47 ohm, 61.9 ohm, 100 ohm and a 33 ohm power LED; the four LED / LDR switches (LDR1 to LDR4) are driven from the footswitch and the OD switch.

## 5. Not read / uncertain
- Exact pot tapers and a few of the attenuator values at the top left (the 470k / 500k / 39k dividers) were read from the inset; the percentages (48 %, 96 %, 87 %, 32 %) are the author's printed notes, not my arithmetic.
- Which of the two clean-channel gain stages is called "Clean preamp 3" in Clean mode (the drawing reuses V3B).
- The Depth control is marked "(Optional)".

## 6. What the code uses, and what it does not
- Used (v93, Slow Burn, Circuit-derived mode only): V1b and V2a both 1.8k / 1 uF; late stack 250k / 1M / 25k / 47k with 470 pF / 0.02 uF; 39k feedback with a 25k presence and 0.1 uF; 2.2k grid stoppers, screens 470 to 475 ohm; supply 497 V, 2 x 200 uF, choke, 10k x 40 uF.
- Not modelled: the **cold clipper** (V2B, 39k unbypassed cathode: asymmetric, level-dependent clipping that gives the SLO's smooth, compressed lead); the **Clean channel** (Slow Burn has Smooth, Lead, Crunch and Sustain, none of which is the clean channel); the **parallel clean + OD interaction in OD mode** (out of phase, no mixing resistors); the **optocoupler switching** (silent, no effect on tone); the **FX loop** (a 99 % attenuator and a recovery stage: the amp's own colour, not modelled).
- Retunes of Slow Burn are held until you pick them (R-list in the briefing doc); nothing in the plug-in changed because of this sheet.
