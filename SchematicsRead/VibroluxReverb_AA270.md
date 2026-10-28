# Fender Vibrolux Reverb AA270 (silverface) - schematic read

Source: 6321f384-fender_vibrolux_reverb_aa270.pdf, TWO sheets:
- Sheet 1 (1756x1454 px, 1-bit raster): Fender Musical Instruments (CBS), Fullerton CA, "Schematic and wiring diagram, Vibrolux Reverb-Amp, model type VIBROLUX-REV-AMP, dwg 045443, Rev F" (revisions B-F incorporate ECO 1050 ... 2053; Rev F ECO 2053 dated 2/17/70(?)). Same drawing style as AB568/AB763 but with the silverface changes. Notice block as AB568: voltages read to ground with electronic voltmeter +/-20 %; resistors 1/2 W 10 %; caps at least 400 V; "vibrato grounded".
- Sheet 2 (2167x1428 px, 200 dpi, much cleaner): Fender/Rogers/Rhodes (CBS) "Schematic - Vibrolux Reverb Amp with Volume Boost Pedal", part no. 014888, rev "-". Six-digit Fender part numbers printed. Capacitors 600 V min, resistors 1/2 W 10 %. This is a later silverface variant (with an extra volume-boost pedal jack and export power-transformer taps) - with hand corrections in grey pencil (25-25, 2200?) on the scan.
Where the sheets differ, both values are given and tagged S1 / S2.
Ground symbols: signal ground triangle and chassis ground; both ground.

## 1. Tubes and heaters
| Position | Type | Function |
|---|---|---|
| V1A/V1B | 7025 | Normal channel stage 1 / stage 2 |
| V2A/V2B | 7025 | Vibrato channel stage 1 / stage 2 |
| V4A/V4B | 7025 | V4A = mixer/ram stage, V4B = reverb recovery (S2 naming; S1 marks both "1/2 7025", cathodes tied at node E) |
| V3 | 12AT7 | reverb driver (halves paralleled) |
| V5A/V5B | 12AX7(A) | vibrato oscillator / lamp driver |
| V6A/V6B | 12AT7 | long-tail phase inverter |
| V7, V8 | 6L6GC | output pair |
| V9 | 5U4GB | rectifier |
Heaters: 6.3 V winding (GRN, GRN) "to all 6.3 V heaters"; two 100 ohm resistors from each side of the winding to ground (artificial centre tap); pilot lamp through 4.7 ohm (S2, marked as "used in some versions"). Mains: 120 V 60 Hz, 2 A 125 V SLO-BLO fuse, AC switch (037960), CSA thermal protector 010873 (some versions), .047 uF/125 VAC ground-switch cap (010023), AC receptacle; export power transformer taps for 100/120/127/220/240/260 V (S2, TR 022723).
Export model transformer part table (S1): TR1 125P31A (022731) / 125P26A (022723); TR2 125C3A (022707); TR3 125A6A (022848); TR4 125A20B (022921).

## 2. Preamp stages
### Normal channel
- Inputs: jack 2 -> 68K -> grid node; grid node -> 68K -> jack-1 contact; jack-1 contact -> 1M to ground (same cross-connect as AB763; S2 shows 68K, 68K, 1M).
- V1A: plate load 100K to +310 V rail (plate +200 V); cathode 1500 ohm + 25 uF/25 V (cathode +1.5 V).
- Tone stack (section 3).
- V1B: plate load 100K 1 W (S2) to +310 V; plate +200 V; cathode tied via node A: S2 shows no separate 820 ohm in the Normal channel (V1B cathode joins the shared 820 ohm + 25 uF/25 V of V2B at node A; S1 marks +1.5 V and node "A" (+1.5/+1.7 V)). 
- V1B plate -> .047 uF -> 220K -> mix node (to the PI-input coupling). 
### Vibrato channel
- Inputs 68K, 68K, 1M.
- V2A: plate load 100K 1 W (S2) to +310 V; plate +200 V; cathode 1500 + 25 uF/25 V (+1.5 V).
- V2B: plate load 100K 1 W; plate +200 V; cathode 820 ohm + 25 uF/25 V (+1.7 V, node A).
- V2B plate -> .02 uF -> 3.3M || 10 pF -> V4A grid (S2: 3.3M and 10 pF in parallel, then 470K reverb-level ladder and ground via 220K); also 500 pF -> V3 reverb driver grid.
### Mixer / recovery
- V4A (mixer): plate +200 V; load 100K 1 W to +310 V; plate coupling .1 uF / 220K network feeds node A and the PI (details of the plate resistor/capacitor ladder at this node (?)). Cathode joins V4B cathode (+1.7 V, 820 ohm + 25 uF/25 V).
- Mix node into PI: .01 uF coupling to the V6A grid.
- Volume-boost (S2 only): 10K and 220K in series along the bus from the Vibrato channel mix node; the volume-boost pedal jack 021956 and the push/pull switch (on vibrato volume pot 014031) inject via 500 pF / 1K.

## 3. Tone stack (both channels, identical)
- 250 pF from plate to treble-pot top; 100K plate -> stack node; .1 uF stack node -> bass-pot top; .047 uF stack node -> bass-pot bottom; 6800 ohm bass-pot bottom to ground.
- S2: TREBLE 250K-J (part 021857), BASS 250K-J (021857), VOLUME 999KHD (014038 normal, 014031 vibrato); "J" taper = Fender code (the S1 scan reads 250K-A / 2-35 for the same pots). The volume value is not directly printed on S2 (999KHD part code): I take 1M from S1's "1M-A" (?).
- BRIGHT switch with 120 pF across the volume pot, both channels; plus a 10M resistor from the bright switch/cap node to ground (S1 and S2: "10M").

## 4. Reverb and vibrato
- Reverb driver V3 (12AT7 halves paralleled): grid via 500 pF; 1M grid leak; plate +425 V into TR4 (022921) primary (BLU/RED), secondary GRN/BLK to REVERB UNIT input jack; cathode: S1 = 2200 ohm + 25 uF/25 V (+8.5 V); S2 = 680 ohm (printed) with +6 V cathode and no bypass shown, and grey pencil notes on the scan that suggest changing to "25-25" cap plus "2200" (hand edit; S1 and the pencil edit agree). Plate supply node B (+460 V S1).
- Recovery V4B (S2) / half-7025 (S1): reverb output jack -> grid; 220K to ground (S1; S2 shows 220K || 2000 pF to ground at this grid); 820 ohm + 25 uF cathode (+1.7 V); plate +200 V, 100K load (to +310 V, S1 "+310V D"); plate -> .003 uF -> REVERB pot 100K-L (021873) -> ground; wiper -> 470K -> mix node (220K to ground on the node) as AB763.
- REV. PEDAL switch grounds the recovery grid (S2: the dashed block "REV. PEDAL").
- Vibrato V5 (12AX7): oscillator V5A: 1M, 1M, .01 uF, .01 uF, .022 uF phase-shift network, 220K plate load (plate +260 V), cathode 2700 ohm + 25 uF/25 V (+2 V); plate -> .022 uF; SPEED 3M-RA (021865) + 100K; V5B driver: +360 V plate (10M from B+, (?)), cathode 100K + 5 uF/50 V [S1 reads "25/50V", S2 prints 5 uF/50 V with grey pencil circle] (+18 V cathode), neon lamp + photoresistor in the same ellipse, 100K from B+ in series with the lamp; INTENSITY 50K-RA (021915) INTENSITY pot with photocell (see below). The 2.2M and the VIBRATO PEDAL jack sit on the oscillator network (pedal shorts the oscillator).
- INTENSITY (sheet 2, read directly): photocell from ground to the INTENSITY 50K-RA (021915) pot wiper; one pot end to boxed tie point A (the shared preamp V1B/V2B cathode node, 820 ohm + 25 uF), the other end to ground. So on the AA270 the optical element varies the preamp cathode node (gain), not the output-tube bias. The separate bias supply on this sheet has its own 15K / 10K-L (OUTPUT MATCHING 010437) / 3.3K / 80 uF network at -48 V, with a 2.2M from the vibrato pedal jack to that area.

## 5. Phase inverter (12AT7 long-tail)
- Coupling: .01 uF from the mix node to V6A grid (S1 and S2).
- Grid leaks: S1 1M (top), 1M (bottom); S2 330K (top), 330K (bottom). Bottom grid: .1 uF/200 V to the left bus.
- Tail: left bus node -> 22K -> node J; J -> cathode resistor to the cathodes: S1 470 ohm, S2 680 ohm; cathodes +100 V (printed on both).
- Left bus: 100 ohm to ground; 820 ohm to the speaker secondary (global NFB).
- Plate loads: 47K 5 %? (read "47K") each, to the +375 V node; plates +270 V (both).
- Outputs: .1 uF each -> 1500 ohm stoppers to 6L6 grids; 100K from each grid to the bias line.

## 6. Output stage
- 2x 6L6GC, fixed bias, cathodes directly to ground (centre ground symbol between tubes); bias line printed -48 V.
- Grid circuit: .1 uF coupling, 1500 ohm stopper, 100K leak to the bias line, plus a 2000 pF cap from each grid (at the stopper/tube side) to ground (RF/oscillation suppression; not on AB763 and not on AB568 (?)).
- Screens: 470 ohm 1 W each from the B+ node; plates +450 V printed (S2; S1 prints 450 V on the OT CT line and 445 V at the node near TR2).
- Output transformer TR3 (022848): primary BLU/RED/BRN; secondary GRN, BLK; EXT SPKR jack, SPKR jack; two 8 ohm speakers drawn in S1 (each "8 ohm", handwritten); handwritten "TOTAL LOAD 4 ohm" (S1) / printed "OUTPUT 40 WATTS, TOTAL LOAD IMPEDANCE 4 OHMS" (S2).
- Global NFB: 820 ohm from the speaker secondary (GRN) to the PI left bus with 100 ohm to ground (both sheets).
- Bias supply: TR1 tap -> 1K -> diode (arrow shows + toward the supply) -> 50 uF/70 V (S1) or 80 uF/75 V (S2, with pencil note "70 .. 100 V") to ground at the -48 V node via 3.3K; S2: "-48V" node between 80 uF/75 V and the 3.3K, with bias pot "OUTPUT MATCHING 10K-L" (010437; adjusts bias, it was labelled "10K-L" in S1) in series with 15K to ground; second 80 uF/75 V on the rectified side. S2 diode 0.5 A/400 V.

## 7. Supply
- TR1: 345 V AC each side to 5U4GB (printed 345/345 V AC; S1 reads 345 V; S2 YEL/RED/RED windings). Rectifier cathode +465 V (S1, printed "+465 V STANDBY OFF" on the line to the standby switch).
- STANDBY switch (037960), then two parallel 16 uF/450 V caps (2x16 uF first filter), choke TR2 (125C3A/022707), then node B 16 uF/450 V (+445 V printed), B -> 4.7K 1 W [S2: 2.2K 1 W] -> node 16 uF/450 V (+375 V), -> 10K 1 W -> node D 16 uF/450 V (+310 V rail for the preamp).
- Printed voltages: rectifier +465 V; OT CT/plates +450 V; B +445 V; +375 V (PI plates and V3 reverb driver at +425 V from the plate node); D +310 V. Heater winding not specified but 6.3 V.

## 8. Not legible / uncertain
- Mixer/recovery plate resistor and coupling arrangement; the exact labelled voltages on V4A.
- The "10M" bleeder arrangement for the BRIGHT switch (printed on both sheets, topology (?)).
- Volume pot values on S2 (part codes); whether the volume-boost circuitry is installed in the base AA270.
- Vibrato V5B cathode cap (5 vs 25 uF), lamp/photoresistor wiring, 10M.
- Revision-letter date on sheet 1; handwriting on sheet 2.
