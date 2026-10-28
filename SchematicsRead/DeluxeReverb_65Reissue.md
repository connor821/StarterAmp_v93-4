# Fender '65 Deluxe Reverb (reissue) - schematic read

Source: `3821f542-Fender_65_Deluxe_Reverb.pdf`, 2 pages, both 1-bit 400 dpi scans. Page 1 (3080x4609 px, rotated 90 deg in the PDF) is the **schematic**; page 2 (4168x6093) is the **PCB service/layout diagram, Dwg 041287 Rev B (5-13-96, EC#1694)** with test-point locations and wire colours, no component values. Very clean, high-resolution scan; nearly everything is legible.
Title block: "SCHEMATIC DIAGRAM, 65 DELUXE REVERB AMPLIFIER", Fender Musical Instruments, 1130 Columbia Street, Brea, CA 92621; **Drawing No. 041285, Rev B**, size D, drawn by B. Desi, created 25-FEB-93, plotted 07-JUL-93; database file 650LXS. Revision table: A "PR#239" 7-2-93; B "LO2732" 11-4-93. Next higher assembly: 65 Deluxe Reverb Guitar Amplifier. Parts are given with **reference designators** (R1...R71, C1...C36) and **test points (TP1...TP39)**.
Notes on the sheet (numbered): (1) V1, 2, 4, 5 are 12AX7A. (2) All resistors 1/4 W 5 % carbon film. (3) 6-digit numbers are Fender part numbers. (4) Fuse 2 A fast-acting in 100-120 V models, F1A in 220-240 V models. (5) U.L. listed E33628, C.S.A. listed 32208. (6) C30 and S2 installed in 120 V units only. (7) CP3-8 not installed in 120 V units. (8) Voltages measured to ground with a DVM with min 1 Mohm AC input impedance and 10 Mohm DC impedance; voltages +/-20 %; line 120 VAC 60 Hz; STANDBY switch S3 on; reverb and vibrato switched off with footswitch; vibrato speed R42 at max CW rotation; all other panel controls at 50 % rotation; **bias set for -37 VDC at TP36**; all DC voltages measured with no input signal; TP29-30 measured with an oscilloscope with no input signal and vibrato switched on; TP6, 8, 9, 11 measured with a 1 kHz sine-wave input to the Normal channel only; all other AC test points measured with a 1 kHz sine-wave input using a x10 probe. (9) AC voltages at TP31 and TP32 measured with an oscilloscope. (10) last instances list. Output test: **22 W minimum RMS into 8 ohm at 5 % THD, 1 kHz input into the Normal channel at J1, all Normal-channel controls full CW, Vibrato controls full CCW, reverb and vibrato off.**
Convention: this sheet uses ohm and kohm printed explicitly, "Fp" prefix = flameproof, "(1W)" etc. as shown. Caps in uF/pF with voltage.

## 1. Tube complement and heaters
| Ref | Tube | Function |
|---|---|---|
| V1 | 12AX7A | Normal channel V1A, V1B |
| V2 | 12AX7A | Vibrato channel V2A, V2B |
| V3 | 12AT7 | Reverb driver V3A + V3B (paralleled) |
| V4 | 12AX7A | V4A = reverb recovery, **V4B = channel mixer / reverb mixer** |
| V5 | 12AX7A | V5A = tremolo oscillator, V5B = modulator |
| V6 | 12AT7 | Phase inverter V6A + V6B |
| V7, V8 | 6V6GT | Power output |
| V9 | **5AR4** | Rectifier |
- Heater: 6.3 V (GRN) winding from T3; **R70 100 ohm 1/2 W and R71 100 ohm 1/2 W** from the two ends of the heater winding to ground (artificial centre tap); pilot lamp LA1 T47. Rectifier filament from separate winding (YEL).
- Power transformer T3 part 041316 (120 V); export power xfmr 041317 (tap table for 100/230/240 V). Output transformer T2 **041318**. Reverb driver transformer T1 **037099**. Reverb pan **063064**. Opto OP1 **037836**. Footswitch assembly 037211. L1 filter choke **022707**. Thermistor TH1 C60-11 5 A (inrush limiter). C30 .01 uF 125 VAC (120 V only). F1 2 A.

## 2. Stage by stage

### Normal channel
- **Input**: J1 (jack 1, high) and J2 (jack 2, low). **R1 68k, R2 68k** (each jack through its own 68k to the V1A grid node), **R3 1 Mohm** grid leak from the jack-1 side to ground.
- **V1A**: plate load **R4 100k 1/2 W** to supply node **X (+261 V, TP4)**; cathode **R5 1.5k || C1 22 uF 25 V**, cathode **+1.3 VDC (TP7)**. AC at plate 1.6 VAC (TP8, with the 1 kHz test condition).
- **Coupling to the tone stack**: none in series (plate node goes straight to C2 250 pF / R6 100k).
- **Volume R10: 1 Mohm, taper "30A" (audio)**. No bright cap on Normal.
- **V1B**: plate load **R11 100k 1/2 W** to X; cathode **R24 820 ohm || C11 22 uF 25 V** (shared with V2B), cathode **+1.4 VDC (TP10)**.
- **Output**: V1B plate -> **C5 .047 uF 400 V** -> **R12 220k** -> **PI input node** (the node that goes to C25 .001 uF). This node is the same node that the mixer plate (V4B) reaches via R35 220k.

### Vibrato channel
- Input: J3 (jack 1) / J4 (jack 2); **R13 68k, R14 68k, R15 1 Mohm**.
- **V2A**: plate load **R16 100k 1/2 W** to X; cathode **R17 1.5k || C6 22 uF 25 V**, **+1.3 VDC (TP13)**.
- **Volume R22: 1 Mohm "30A"**; **bright cap C10 47 pF 1 kV** from the treble/stack output to the V2B grid (across the volume pot top-to-wiper).
- **V2B**: plate load **R23 100k 1/2 W** to X; cathode R24 820 ohm || C11 22 uF (shared with V1B).
- **Output**: V2B plate -> **C12 .022 uF 400 V** -> mixer grid node (the node with R25 3.3 Mohm || C13 10 pF 1 kV). The same V2B-output node also feeds the **reverb driver via C14 500 pF 1 kV**.
- Mixer is V4B: its grid network is **R25 3.3 Mohm in parallel with C13 10 pF/1 kV** from the C12 node to the V4B grid, **R33 220k** from V4B grid to ground, and **R32 470k** from the REVERB pot wiper line into the V4B grid node.

## 3. Tone stack (both channels, identical values)
- **C2/C7 250 pF 1 kV** from the plate node to the top of the TREBLE pot.
- **R6/R18 100k** from the plate node to the stack node; **C3/C8 .1 uF 400 V** from the stack node to the bass-pot top; **C4/C9 .047 uF 400 V** from the stack node to the bass-pot bottom; **R9/R21 6.8k** from the bass-pot bottom to ground.
- **TREBLE R7/R19 250k "30A"** (audio), **BASS R8/R20 250k "10A"** (audio, 10 = different resistive-track code; 30A and 10A as printed), VOLUME 1M "30A".
- (The "30A/10A" codes are part of the Fender pot-code naming; taper letter A = audio.)

## 4. Reverb
- **V3 12AT7 driver, both halves paralleled**: grid via **C14 500 pF** from the V2B-output node; grid leak **R26 1 Mohm**; cathodes **R27 2.2k 1 W || C15 22 uF 25 V**, cathode **+7.1 VDC (TP18)**; plate to **T1 (037099)** primary (RED to supply **Z = +394 V**, CP21); AC at plate 60 VAC (TP19) (test condition). T1 secondary (GRN/BLK) -> J5 (reverb pan input RCA, white), 900 mVAC (TP20).
- **Reverb pan 063064** (4 spring) in/out, return to J5 output, min 1.4 mVAC (TP21).
- **Recovery V4A**: grid from the pan output with **R28 220k to ground** (no extra cap across it); cathode **R30 820 || C16 22 uF 25 V**, **+1.4 VDC (TP22)**; plate load **R29 100k 1/2 W** to X; plate -> **C17 .0033 uF 400 V** -> **R31 100k "B" (linear) REVERB pot**; the REVERB wiper -> R32 470k -> V4B grid node (see above). Min 80 mVAC at the V4A plate (TP23).
- **Mixer V4B**: plate load **R34 100k 1/2 W** to X; plate -> **C18 .1 uF 400 V** -> **R35 220k** -> PI input node (the same node that gets V1B via .047 uF + R12 220k). Plate AC 3.8 VAC (TP25); grid 115 mVAC (TP24). Cathode: V4B pin 8 line appears to run to the V1B/V2B common cathode node (R24 820 ohm || C11 22 uF) but the routing is dense (?).
- Reverb footswitch (J6 RING, grey) shorts the reverb signal (reverb pot output/R32 junction) to ground when pressed.

## 5. Tremolo
- **V5A oscillator**: plate load **R43 220k 1/2 W** to Z (+394 V); cathode **R44 2.7k || C23 22 uF 25 V**; DC **+1.8 V on / ~0 V off (TP27)**; plate waveform 300 V pk-pk (TP29).
- RC phase-shift network: **C20 .01 uF 400 V**, **C21 .01 uF 400 V**, **C22 .022 uF 400 V**, **R39 1M**, **R40 1M**, **R37 390k**, **R38 2.2M**, **R41 100k** and **R42 3 Mohm "10C" SPEED pot** (variable, series with R41 to ground; speed range about 0.5-5 Hz per the '68 manual). C19 **4.7 uF 50 V** on the node to the -49 V rail.
- **V5B modulator**: plate load **R45 10 Mohm** to Z; cathode **R46 100k 1/2 W || C24 22 uF 25 V**; DC **+11 V on / ~0 V off (TP28)**. V5B plate feeds the **OP1** LDR opto assembly (037836, pins 1-4): lamp (neon/pins 1-3) driven from the V5B plate; **R47 100k 1/2 W** from Z to opto pin 2; LDR side to **R48 50k "80C" INTENSITY** (audio-reverse type code as printed).
- Where the opto output (OP1 pin 4 -> R48 wiper -> ground) attaches to the audio path: **I could not trace it with certainty** (the line leaves R48 and runs a long way left across the sheet toward the PI input region). The '68 manual states the same circuit "modulates the input impedance of the non-inverting input of the tube power amp" (PI input node, which receives both channels in these reissues). Treat as PI-input shunt, both channels (?).
- **Footswitch**: J6 TIP (white) through **R36 10k** to the -49 V rail; vibrato "off" = -6.8 V at TP26 (applied to the oscillator network), "on" = -0.20 V.

## 6. Phase inverter (V6, 12AT7, LTP)
- **V6A grid**: **C25 .001 uF 400 V** from the PI input node (R12/R35 junction); **R49 1 Mohm** from the grid to the bias node (**+... node of R50/R51/R52**). 
- **V6B grid**: **C26 .1 uF 250 V** from the NFB node; **R52 1 Mohm** to the bias node.
- **R51 470 ohm** from the bias node to the common cathodes (**+68 VDC, TP39**); **R50 22k 1/2 W** from the bias node to the NFB node; **R53 47 ohm** NFB node to ground; **R64 820 ohm** from the OT secondary (speaker line) to the NFB node = the global NFB.
- Plate loads: **R54 82k 1 W** (V6A) and **R55 100k 1 W** (V6B), both to **Y = +311 V (TP3)**. AC at plates: 22 VAC (V6A, TP34), 23 VAC (V6B, TP35). Plate DC values not printed.
- Output coupling: **C27 .1 uF 630 V** (V6A) and **C28 .1 uF 630 V** (V6B). **R56 220k** and **R57 220k** grid leaks to the bias line (-37 V). **R60 1.5k** and **R61 1.5k** grid stoppers. (No 1200 pF caps.)
- NFB ratio: R53/(R64+R53) = 47/867 = 5.4 % (-25.3 dB), same as blackface.

## 7. Output stage
- V7, V8 **6V6GT** push-pull, fixed bias, cathodes grounded; **screen resistors R62 470 ohm 1 W (flameproof) and R63 470 ohm 1 W (flameproof)**; screens and plates from the T2 centre tap B+ (RED). Plate voltage +391 VDC (TP37) and 174 VAC at plates (TP37/TP38 test condition).
- **Bias**: **R58 10k "B" BIAS ADJ pot** (P18 gray, P19 white wiper, P20 black) with **R59 10k 1/2 W** to ground; **bias rail "C-" = -49 VDC (TP5)**, adjust to **-37 VDC at TP36**; **C29 22 uF 63 V** on the wiper rail. Bias supply: T3 RED/BLU winding -> **CR1 1N4003 -> R69 22 ohm (flameproof fuse resistor) -> C36 100 uF 100 V -> C-**.
- **Output transformer T2 041318**: primary Z not printed; secondary to **J7 (speaker) and J8 (EXT speaker)**, 8 ohm speaker. **Global NFB: R64 820 ohm** from the 8-ohm secondary (the BLK/GRN secondary line) to the PI NFB node.

## 8. Supply
- **V9 5AR4**, T3 RED windings; filament via YEL windings; TP1 **+396 VDC** at rectifier cathode (P10).
- Reservoir: **C31 220 uF 100 V in series with C32 47 uF 500 V**, with balancing resistors **R65 100k 1/2 W (across C31)** and **R66 470k 1 W (across C32)** - i.e. about 38.7 uF effective, series-connected pair.
- **STANDBY S3**, then **P12, SP1, L1 choke (022707), SP2** to **node Z (TP2 +394 VDC)** = reverb driver plate, OT centre tap/screens (+391), tremolo, **C33 22 uF 500 V**.
- **R67 10k flameproof 1 W** from Z to **Y: +311 VDC (TP3)**, **C34 22 uF 500 V**. **R68 10k flameproof 1 W** from Y to **X: +261 VDC (TP4)**, **C35 22 uF 500 V**. X feeds all preamp plate resistors (R4, R11, R16, R23, R29, R34).
- DC at test points: TP1 +396, TP2 +394, TP3 +311, TP4 +261, TP5 -49, TP36 -37, TP37 +391, TP39 +68.
- Approximate waveform at TP30 (tremolo modulator plate): swings between +330 V and +260 V, 30 ms/60 ms period markings.

## 9. Not legible / uncertain
- Page 2 is the PCB service diagram (Dwg 041287 Rev B); no values transcribed from it.
- Which node V4B's cathode returns to (shared 820 ohm / 22 uF with V1B/V2B vs own) - I read it as shared, but the drawing is dense.
- Exact connection of the OP1 LDR to the power-amp input (see section 5): I read it as the NFB/V6B-grid node.
- Pot-code suffixes (30A, 10A, 80C, 10C, B) are printed as codes only.
- Plate DC voltages of preamp and PI stages are not printed (only AC TPs, cathode DCs and supply nodes).
- Speaker impedance printed "8 ohm" next to the speaker symbol; OT primary impedance is not printed.
