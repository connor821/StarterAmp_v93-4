# Fender Deluxe Reverb-Amp AA1270 (silverface, later) - schematic read

Source: `8d923f1c-Fender-Deluxe-Reverb-A1270-Schematic.pdf`. 1 page, one embedded **72 dpi 1262x1484 palette scan** (low resolution, heavy aliasing; I rotated it 90 deg and upsampled to read). Title: "FENDER MODEL DELUXE REVERB-AMP A1270 SCHEMATIC NOTICE" (Fender code AA1270). Title block: "Schematic & Wiring Diagram, Deluxe Rev-Amp", **Dwg No. 045450**, size C, rel. no. 520, **Rev "6"** (the AA1172 sheet is Rev H), **Sheet 1 of 2** (sheet 2 not in the file). "Fender Musical Instruments, CBS Musical Instruments ... Fullerton, California" footer, as AA1172. Sheet notes: voltages +/-10 %, resistors 1/2 W 10 %, caps at least 600 V unless specified.

**Result of a value-by-value comparison: every legible value on this sheet equals AA1172** (details below). The one possible difference is the voltage printed at node C, which is not readable here.
All values below were re-read on this scan, not copied from AA1172, except where marked "(not re-verified)". See `DeluxeReverb_AA1172.md` for full narrative; this file lists the values.

## 1. Tube complement and heaters
V1 7025 (Normal), V2 7025 (Vibrato), V3 12AT7 (reverb driver, paralleled), V4 7025 (V4A mixer + V4B recovery), V5 12AX7A (oscillator/modulator), V6 12AT7 (PI), V7/V8 6V6GT, V9 **5U4GB**. Heaters: 6.3 V winding with **two 100 ohm resistors to ground** (artificial CT); "TO ALL 6.3 V HEATERS AND PILOT LIGHT". **2 AMP fuse** (SLO BLO), ground switch with **.047 uF 600 V**.

## 2. Stages
**Normal (V1):** inputs 68k / 68k into grid node, 1M jack-1 to ground. V1A plate **+180 V**, plate load **100K 1W** (printed "100K,1W"), cathode **1500 || 25 uF, +1.3 V**. Volume **1M-J**. V1B plate **+180 V**, plate load **100K 1W**, cathode **820 || 25 uF** (node A, shared with V2B), +1.3 V. Out: **.047 uF -> 220K** to node M.
**Vibrato (V2):** same inputs; V2A plate **+170 V**, load 100K 1W, cathode 1500 || 25 uF +1.3 V; **47 pF** bright cap on volume (1M-J); V2B plate **+170 V**, load 100K 1W, cathode **820 || 25 uF** (A), +1.3 V; out **.022 uF** to V4A grid node. (Preamp supply: "100K,1W" resistors printed on the line between V1A/V2A as on AA1172.)

## 3. Tone stack (both)
250 pF treble cap; 100K slope; .1 uF and .047 uF; **6800 ohm** bass-bottom to ground; **TREBLE 250K-J, BASS 250K-J, VOLUME 1M-J**. Identical to AA1172.

## 4. Reverb
V3 12AT7: **500 pF** coupling, **1M** grid leak, cathode **2200 || 25 uF, +8.7 V**, plate **+410 V** to **TR4** (node B). Recovery (V4B): grid leak **220K with 2000 pF across** (printed "2000 PF"), cathode **820 || 25 uF, +1.3 V** (E), plate **+170 V**, plate load **100K 1W** to node D, **.003 uF** -> **100K-L REVERB** -> wiper -> **470K**; **220K** to ground at the mix node; **3.3M || 10 pF** into the mixer grid; V4A plate **+180 V**, cathode +1.3 V node E. Same as AA1172.

## 5. Tremolo
V5 12AX7A: V5A plate **+270 V** (220K load), cathode **2700 || 25 uF, +2.1 V**; V5B plate **+345 V**, cathode **100K || (5 uF/50 V ?)**, cathode **+17.0 V**; **10M**, neon lamp + **100K**; **50K-RA INTENSITY**; SPEED **3M-RA** + **100K**; 1M, 1M, 2.2M, .01, .01, .022; vibrato pedal jack. Same as AA1172 (the V5B cathode-cap digit is hard to read here: "5/50" (?)).

## 6. Phase inverter
.01 uF input coupling, **330K** x 2 grid-bias resistors, bias node **+75.5 V** (printed "+75.5V"), **22K** to NFB node, **470** to cathodes (**+90 V**), **47 ohm** NFB node to ground, **.1 uF-200 V** to V6B grid, **82K / 100K** plate loads, V6A **+190 V**, V6B **+180 V**, node C **"+3?5 V"** (printed; the last digit(s) unreadable on this scan, AA1172 prints +375 V) (?). **820 ohm NFB** from OT secondary.

## 7. Output
6V6GT x2: plates **+415 V**, screen line **+415 V** via **470 ohm 1 W**, OT CT **+420 V**; **1500** grid stoppers, **220K** grid leaks, **1200 pF** grid caps, bias **-35 V** (printed "-35V"); bias chain 470 ohm 1 W -> diode -> **50/70** cap -> **10K-L** pot + 10K. TR3, **8 ohm** speaker, EXT SPKR jack. Same as AA1172.

## 8. Supply
5U4GB, 330 VAC per side; standby switch; **two 16 uF 450 V**; TR2 choke; B (16/450) -> **2.2K 1W** -> C (16/450) -> **10K 1W** -> D (16/450). Printed: +420 V, +415 V, +410 V (V3), node C (see 6), +345 V, +270 V. Export transformer table identical to AA1172 (TR1-125P33A 022749, TR1-125P23B 025130, TR2-125C3A 022707, TR3-125A1A 022640, TR4-125A20B 022921).

## 9. Not legible / uncertain
- Node C voltage (see 6).
- V5B cathode cap value.
- Sheet 2 of 2 is not in the file (probably the layout/wiring page).
- Whether Rev "6" vs H introduces any change in values: none seen.
- Fine print like the V4A plate/.1 uF/220K topology is as uncertain as on AA1172.
