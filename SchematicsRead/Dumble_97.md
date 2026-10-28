# Dumble '97 ("1997 Humble Overdraft Specimen, Revision 1.2")

Source image: `dumble97-1.png` (3667x2834, a clean redrawn schematic, all printed values legible). The sheet's legend says "Red indicates confirmed value or component; Blue indicates probable value or component"; the image was supplied in 2-bit grayscale, so I cannot tell which values are confirmed and which are probable. Every value below is as printed; the italic grey numbers in boxes are DC voltages printed on the schematic. Computed values come from my own nodal solver of the plugin's tone-stack topology and RC formulas; they are estimates.

## 1. Tube complement

| Ref | Type | Stage |
|---|---|---|
| V1B | 12AX7 | Input stage, feeds the clean tone stack |
| V1A | 12AX7 | Second clean stage, after the volume pot |
| V2B (first triode) | 12AX7 | OD channel stage 1 (printed "V2B", the pair is labelled V2B twice; the first is probably V2A, (?)) |
| V2B (second triode) | 12AX7 | OD channel stage 2 |
| V3A, V3B | 12AX7 | Phase inverter pair (long-tail style, separate plate resistors) |
| V4, V5 | EL34 | Push-pull power amp |
| Rectifier | four silicon diodes | Full-wave with center-tapped 345-0-345 V winding |

## 2. Clean channel stage by stage

### V1B
- Input jack, grid stopper 22k, grid leak 1.0M to ground.
- Plate load 220k to B+5 (323 V); plate 209.6 V, cathode 1.716 V printed.
- Cathode: 3k3 parallel with 5.0 uF (corner 1/(2 pi 3.3K 5 uF) = 9.6 Hz).

### Tone stack (the clean stack is a Dumble-style wiring; not a standard TMB)
Printed values:
- Plate side: 330 pF (treble cap) to the 250kB treble pot top; 150k slope resistor from the plate node down to the bass/mid network.
- Capacitors: 0.1 uF (from the slope node to the treble pot wiper side), 0.01 uF (to the middle pot), 0.001 uF across the bass pot (printed beside the bass pot as ".001uF").
- Pots: Treble 250kB; Bass 500kA; Middle 250kA (marked "ccw", wired as a variable resistor, not a divider); 9k8 resistor to ground at the bottom of the bass pot.
- Volume 1MA follows, with a 390 pF capacitor in parallel with a 220k resistor between the stack output and the volume pot top (a treble bleed, the "bright" network).
Computed with the solver (source 220k in parallel with an estimated 62k tube rp = about 48K, load 1M volume, bass 500k, mid 250k as rheostat, 9k8 to ground, 1 nF across the bass pot, all pots linear position because the taper is mostly printed as A/B but the actual placement is unknown): noon insertion loss mean 100 Hz to 5 kHz about -14.9 dB, -15.0 dB at 1 kHz, notch about -18.9 dB near 420 Hz. All-10 minimum about -9.4 dB near 350 Hz, maximum -4.0 dB. This clean stack is not among the plugin's menu stacks.

### V1A
- Plate load 150k to B+5, plate 206.0 V, cathode 1.788 V; cathode 3k3 parallel with 5.0 uF.
- Grid from the volume wiper.
- Output coupling 0.047 uF into the "1MA Clean Master" pot, then a channel-select switch to "To Output Section" (the PI).

## 3. Overdrive channel (OD)
- The OD path takes the V1A output (before the clean master) through a fixed 100kB resistor and a 100kB OD Trim pot (printed "100kB OD Trim"), then 68k into the first OD triode grid.
- First OD triode: plate 180k to B+4 (361 V), plate 227.5 V, cathode 2.03 V, cathode 2k7 parallel with 5.0 uF (corner 12 Hz). Coupling 0.0022 uF into a 100k resistor and a 250kA Drive pot (the 0.0022 uF with 100k is a 720 Hz high-pass: 1/(2 pi 100K 0.0022 uF)).
- Second OD triode: 180k grid stopper from the Drive wiper; plate 120k to B+4, plate 229 V printed; cathode 1k8 parallel with 1.0 uF (corner about 88 Hz); cathode 2.03 V (printed, (?) for which triode it belongs to).
- OD tone stack: 330 pF with a 33k resistor in the treble leg, 250kB treble, two 0.022 uF capacitors (bass and mid), 1MB bass, 20kB mid; then a 1MA OD Level pot returning to the switch. Computed (source 120k parallel about 62k = about 41K, load 1M): noon mean about -12.5 dB, -16.3 dB at 1 kHz, notch about -16.7 dB near 780 Hz; all-10 minimum -13.8 dB near 520 Hz. This is a JCM-800-like TMB (330 pF, 22 nF, 22 nF, 33k slope) with a 1MB bass and a 20kB mid, so the plugin's Marshall/Silverface stack types are close to it.
- No diodes or cold clippers anywhere in the OD path; the drive is two cascaded 12AX7 stages with an unbypassed-ish Drive pot, and gain/overload is tube saturation. The channel switch is a DPDT that selects clean master or OD level.

## 4. Phase inverter and power amp
- PI: V3A (plate 91k to B+3 (433 V), 303 V) and V3B (plate 110k, 317 V); common cathode node 60.9 V; input signal from the preamp via 0.01 uF (V3A) and 0.1 uF (V3B), grid leaks 1M each; 24k and 820R resistors tie the grid/cathode nodes (the printed values are 24k, 820R, 1M and 1M; exact connection (?)).
- Coupling to the EL34 grids: 0.1 uF each into 200k grid leaks to the bias line, through 5k1 grid stoppers.
- Output tubes V4 and V5, EL34, screen resistors 1K/5W each from B+2 (458 V) to pin 4.
- Bias: 1.2k/1W, a diode, 100 uF/100 V, Bias Adj 10kB, 15k to ground; bias voltage not printed (?).
- Negative feedback: 4k7 from the speaker (secondary) to the PI input rail (the node that carries the input signal from the preamp and the 0.01 uF / 0.1 uF PI coupling caps). A presence network hangs on that rail: 390R in series with a 2kB presence pot (wired as a rheostat) and 1.0 uF to ground. The shunt impedance seen by the 4k7 is 390 ohm + the pot (0 to 2k) + the 1 uF reactance (1 uF is 159 ohm at 1 kHz and 1.6K at 100 Hz). Estimated feedback fraction (ideal node): about -22 dB at high frequencies with the pot at 0, about -9 dB with 2k, and about -12 dB at 100 Hz with the pot at 0 because the 1 uF is a larger impedance there, so the feedback is stronger in the bass than in the treble. These are estimates; the preamp source impedance in parallel at that node is ignored (it is large, tens of K).
- Output transformer: center tap to B+2 through the two 1K/5W; speaker jack on the secondary, grounded.

## 5. Supply
- Transformer secondary 345-0-345 V, four rectifier diodes, standby switch; B+1 465 V after the 200 uF/300 V x2 (with 330k/1W balance resistors), choke; B+2 458 V (47 uF/350 V x2, 470k/1W balance); 3k/2W to B+3 433 V (47 uF/250 V x2); 12k/2W to B+4 361 V (47 uF/250 V x2); 10k/2W to B+5 323 V (47 uF/250 V x2). Each node has a 470k/1W balancing pair.
- Mains 120 V ac, black and white, 2.0 A SB fuse and power switch; heaters with two 100R humdinger resistors; bias supply tapped from the HV winding via the 1.2k/1W resistor.

## 6. Uncertain or illegible
- Which values are marked red (confirmed) versus blue (probable) in the original; the duplicate "V2B" labels; the PI cathode resistor connections; the bias voltage; the exact tap of the 4k7 NFB return.
