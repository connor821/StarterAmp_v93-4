# Fender Deluxe Reverb-Amp AB868 (late blackface, CBS-era drawing) - schematic read

Source: `da5837ed-fender_deluxe_reverb_ab868.pdf` (1 page, low-resolution grey/1-bit scan 930x775 pt; legible for most values, a few hand-written revision marks). Title "DELUXE REVERB-AMP AB868 SCHEMATIC NOTICE". Title block: "Fender Musical Instruments, Santa Ana, California" / "Schematic & Wiring Diagram Deluxe Rev-Amp AB868", **Dwg No. 045450, Rev C**, drawn by R. Rivera (date partly legible, "5-24-68"?). Footer: "Fender Musical Instruments, CBS Musical Instruments, a division of Columbia Broadcasting Systems Inc., Fullerton California USA".
Notes on sheet: voltages read to ground with electronic voltmeter, +/-20 %; resistors 1/2 W 10 % unless specified; caps at least 400 V unless specified.
Convention as in the AB763 files (Fender "1500" = 1.5k etc.).

**AB868 is electrically the AB763 circuit with a handful of changes** - listed in section 10 and in `DeluxeReverb_revisions.md`.

## 1. Tube complement and heaters
V1 7025 (Normal), V2 7025 (Vibrato), V3 12AT7 (reverb driver, halves paralleled), V4 7025 (mixer half "V4A" + recovery half "V4B"), V5 **12AX7A** (oscillator/modulator), V6 12AT7 (PI), V7/V8 **6V6GT** (output), V9 **5U4GB** (rectifier, replaces GZ34).
- Heaters: 6.3 V winding, no centre-tap arrow: **two 100 ohm resistors** (one from each end of the 6.3 V winding to ground) form an artificial centre tap. "TO ALL 6.3 V HEATERS AND PILOT LIGHT".
- Mains: 120 V 60 Hz, ground switch with **.047 uF 600 V**, AC switch, **2 AMP SLO BLO fuse** ("2 AMP FUSE SLO BLO"), 330 VAC per side to 5U4GB.

## 2. Stage by stage
### Normal channel (V1, 7025)
- Input: jacks 1/2, 68k each into the common grid node; 1M from jack-1 node to ground.
- V1A: plate **+180 V**, plate load **100k 1 W** (printed "100K, 1W"); cathode **1500 ohm || 25 uF**, +1.3 V.
- Tone stack as section 3; Volume **1M-A (2-35)**; no bright cap.
- V1B: plate +180 V, plate load **100k** (unspecified wattage, "100K"); cathode to node "A" = **820 ohm || 25 uF** (shared with V2B), +1.3 V.
- Output: **.047 uF** -> **220k** -> mix node M (plate node of V4A mixer, which feeds .001 uF to the PI).
### Vibrato channel (V2, 7025)
- Input as Normal (68k/68k; 1M).
- V2A: plate +170 V, plate load **100k 1 W** (printed "100K,1W" on the shared line), cathode 1500 ohm || 25 uF (+1.3 V).
- Volume 1M-A (2-35); **47 pF bright cap** across the volume pot upper half.
- V2B: plate +170 V, plate load 100k; cathode 820 ohm || 25 uF (node A), +1.3 V.
- Output: **.02 uF** -> V4 mixer grid node.

## 3. Tone stack (both channels)
250 pF (treble cap) from plate node to treble top; 100k from plate node to stack node; .1 uF from stack node to bass top; .047 uF from stack node to bass bottom; **6800 ohm** bass bottom to ground; TREBLE 250K-A, BASS 250K-A (2-35), VOLUME 1M-A. Identical to AB763.

## 4. Reverb
- V3 12AT7 driver, both halves in parallel; **500 pF** from the mixer-grid node (Vibrato channel output node) to the V3 grids; grid leak **1M** to ground; cathodes **2200 ohm || 25 uF**, +8.7 V; plate +410 V into **TR4** primary (to node B). Reverb pan in/out as AB763.
- Recovery half (V4B on this sheet, lower 7025): grid leak **220k** to ground (**no 2000 pF across it** on AB868), cathode **820 ohm || 25 uF** (+1.3 V, node E), plate **+170 V**, plate load **100k 1 W** to node D, coupling **.003 uF** -> REVERB pot **100K-L** -> wiper -> **470k** -> mixer grid node; **220k to ground** at that node.
- Mixer half (V4A, upper 7025): grid node fed by V2B via .02 uF + (3.3M || 10 pF) and by the 470k wet path; 220k to ground; plate +180 V; plate load 100k; cathode node E; plate -> .1 uF -> node P -> 220k -> node M. Same as AB763. Reverb pedal switch shorts the recovery grid.
- Printed 3.3M || 10 pF and 220k visible; **no change from AB763** in these values.

## 5. Tremolo
V5 12AX7A. Values as AB763: oscillator plate +270 V (220k load), cathode 2700 ohm || 25 uF (+2.1 V); modulator plate +345 V, cathode 100k || 50 uF? (printed "5 / 50" style, cathode +17.0 V - the cap value is hard to read: "50" on AB868 vs "25" on AB763 (?)); 10M and neon lamp + 100k; 1M, 1M, 2.2M, .01, .01, .02, 3M-RA speed + 100k; **50K-RA INTENSITY**; **10k** from the intensity pot area to ground (visible beside the bias block). Pedal jack at the oscillator input. Photocell on node P as in AB763.
- Bias block, see 7.

## 6. Phase inverter V6 12AT7 (long-tailed pair)
- .001 uF to V6A grid; 1M grid-to-bias node; **22k** from bias node to NFB node; **470 ohm** from bias node to the common cathodes (+77 V); node printed "+75.5 V" (first digit slightly smudged, AB763/AA1172 both print +75.5 V); 1M for V6B; .1 uF - 200 V coupling from the NFB node to V6B grid; **47 ohm** NFB node to ground.
- Plates: **82k 5 %** and **100k 5 %** to node C (**+325 V**); V6A plate +170 V; V6B plate +180 V.
- **NFB: 820 ohm** from the OT secondary to the NFB node (same as AB763).
- Hand-written ink on this scan: next to the lower 1M grid resistor there is a circled mark reading roughly "330K", with an arrow toward the upper 1M. Typed values remain **1M, 1M** (AB763-like); the handwriting indicates an in-progress change to 330K, which is what AA1172 prints. Treat typed 1M as the AB868 value and the 330K as an ECO note (?).
- PI input coupling typed **.001 uF** (AA1172 prints .01).
- Node C printed **+325 V** (AA1172: +375 V); V6A plate +170 V, V6B plate +180 V; cathodes +77 V; bias node "+75.5 V" (printed).

## 7. Output stage
- 2 x 6V6GT; plates **+415 V**; screen line **+415 V** (printed vertically), screens through **470 ohm 1 W** each; OT centre tap printed **+420 V**; cathodes grounded; fixed bias.
- Grid stoppers **1500**; grid leaks **220k 5 %** each to the bias line; **1200 pF** capacitor from each 6V6 grid (after the 1500) to ground - **new on AB868** (not on AB763).
- Bias: **-35 V** printed. Source: 330 VAC lead -> **470 ohm 1 W** -> diode ("-"/"+" as drawn) -> filter **50 uF / 70 V** (printed "50" and "70", the AB763 had 25/50) -> **10K-L** bias pot (10k to ground on its low end), wiper = bias line (the pot marked with a filled-circle callout).
- OT: TR3 (export P/N 125A1A); **8 ohm speaker** printed ("8 ohm SPKR"); EXT SPKR jack. Primary Z not printed.
- NFB: 820 ohm (see 6).

## 8. Supply
- 5U4GB, 330 VAC each side. Standby switch -> **two 16 uF 450 V** in parallel -> choke **TR2** (P/N 125C3A) -> node B **16 uF 450 V** -> **10k** -> node C **16 uF 450 V** -> **10k** -> node D **16 uF 450 V**. Printed resistor values on this scan: both "10K"; **a hand-written "22K"/"2.2K"-like note is inked next to the C-D resistor** (no note by the B-C one) - a revision mark whose exact value I cannot read (AA1172 prints 2.2k for B-C). Typed values are 10K / 10K. (?)
- Printed voltages: +420 V (OT CT / reservoir), +415 V (plates, screen line), V3 plate +410 V (node B), +325 V (node C), preamp plates +170/+180 V. V5: +270 V, +345 V.

## 9. Not legible / uncertain
- Hand-written revision marks (PI 330K, filter-resistor note) - uncertain.
- V5B cathode capacitor value ("5/50" vs "25").
- OT primary Z, tank impedance not printed.
- Date on the drawing block.

## 10. Differences to AB763 visible on this sheet
5U4GB instead of GZ34; two 100 ohm heater centre-tap resistors instead of grounded CT; 2 A fuse instead of 1 A; 1200 pF caps added at power tube grids; bias filter cap 50/70 instead of 25/50; "100K 1W" plate loads on V1A/V2A; 8 ohm speaker printed. V5 12AX7A; same PI, same preamp, same reverb values, same -35 V bias.
