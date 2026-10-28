# Fender "Reverb" outboard unit, Model 6G15 - schematic read

Source: 1a1875ef-fender_6g15_schematic.pdf, 2 pages (embedded 130-137 ppi bitmaps, rendered at 200 dpi; clean, legible). Page 1 = schematic "Fender Reverb Schematic, Model 6G15" (drawing code H-FA); page 2 = chassis layout (same values, used to cross-check; it shows pot values and the voltages A at the sockets).
Notice: voltages read to ground with electronic voltmeter, +/-20 %; all resistors 1/2 W unless specified (layout page: 10 % tolerance); all capacitors at least 400 V unless specified. Transformers: TR1 68319-A (power), TR2 125A12A (reverb driver transformer).
This is an outboard, solid-state-rectified, line-level spring-reverb unit with a built-in dry/wet mixer: input jack -> dry path and wet path -> mixer pot -> output jack. Controls: DWELL (250K-L), TONE (50K-L), MIXER (250K-L); jacks INPUT, OUTPUT, reverb CUT OFF PEDAL.

## 1. Tubes, heaters, rectification
| Tube | Section | Function |
|---|---|---|
| 12AT7 | triode 1 | input / gain stage |
| 12AT7 | triode 2 | dwell-controlled driver-stage input amplifier |
| 6K6GT | pentode (triode-strapped? see below) | spring driver power stage, plate to TR2 |
| 7025 (12AX7 type) | triode A | recovery amp for the spring output |
| 7025 | triode B | dry-path buffer / mixer cathode follower |
Heaters: TR1 low-voltage winding drawn with centre tap to ground, "to all heaters". Rectifier: three solid-state diodes in series (polarity marked) on the HV winding - no tube rectifier. Pilot light on layout. Fuse 3/4 A in the AC line, AC switch.
Note: the 6K6GT is drawn as a pentode (plate, screen, grid, cathode); the screen goes to the +295 V end of the TR2 primary (not through a resistor), so it is a true pentode connection with screen at +295 V.

## 2. Signal path (all values as drawn)
### Input
- INPUT jack; 1M resistor from the input node to ground through the jack's shorting contact (input load/grid-leak). Input node goes to (a) grid of 12AT7 triode 1 (b) the dry path .05 uF.
### 12AT7 stage 1
- Grid at the input node (no stopper). Cathode 1500 ohm with 25 uF bypass (cathode +1.8 V). Plate load 100K in series with 10K to the +250 V rail (plate at +105 V); the 10K is the lower resistor, the plate output is taken from the 100K/10K junction.
- Output: .01 uF from the 100K/10K junction to the DWELL pot top.
### DWELL
- DWELL 250K-L (linear taper "L"): top fed via .01 uF, bottom to ground, wiper to 12AT7 triode 2 grid. Acts as a level control into the driver chain (more dwell = more drive into the tank).
### 12AT7 stage 2
- Grid at DWELL wiper. Cathode 1500 ohm, NOT bypassed (cathode +2 V, local feedback). Plate load 100K to the +250 V rail; plate +120 V. Plate -> .002 uF -> 6K6GT grid.
### 6K6GT spring driver
- Grid: .002 uF coupling (the small cap makes the driver highpass; see below), 220K grid leak to ground. Cathode 1000 ohm 2 W + 25 uF bypass (cathode +24 V). Plate +285 V into TR2 primary; other end of the primary and the screen at the +295 V node (B+ after the choke). The primary drop of about 10 V (295 to 285) indicates the DC resistance of the primary.
- TR2 (125A12A) secondary: 2-terminal winding, one side to ground, other side to the REVERB UNIT INPUT jack (shielded lead, cut-off pedal shown on the same sheet). No transformer ratio, impedance or the tank's impedance are printed. The tank itself is the dashed "REVERB UNIT" with INPUT and OUTPUT connectors; a CUT OFF PEDAL switch (dashed) can ground the reverb output.
- Highpass corner of the driver coupling: .002 uF into 220K grid leak = 1/(2 pi R C) = 362 Hz (computed, not printed).
### Recovery (7025 triode A)
- REVERB UNIT OUTPUT -> grid directly (no grid leak drawn; the cut-off pedal grounds the node). Cathode 1500 ohm + "6" uF/150 V bypass (the layout reads "6 / 250" and the schematic "6 / 150"; I take 6 uF, voltage rating (?)); cathode +1.2 V. Plate load 100K to the +250 V rail; plate +160 V.
- Plate -> .1 uF -> TONE pot 50K-L (top). TONE pot bottom -> .01 uF -> ground; TONE wiper -> .00025 uF (250 pF) -> top of the MIXER pot. So TONE forms a 50K shunt/series network that controls high-frequency content of the wet signal.
### Dry path and buffer (7025 triode B)
- Input node -> .05 uF -> grid of 7025 B. Grid bias network: 2.2M from the +250 V rail to the grid and 2.2M from the grid to ground (grid at +120 V as printed). Plate directly to the +250 V rail (+250 V). Cathode load 100K to ground (cathode +130 V). Cathode -> .1 uF -> bottom end of the MIXER pot. This is a cathode follower.
### MIXER
- MIXER 250K-L (linear): one end fed by the wet signal (via .00025 uF), the other end fed by the dry signal (via .1 uF), wiper to the OUTPUT jack. A crossfade between wet and dry.
- Computed (not printed): dry .1 uF into the 250K pot = 6.4 Hz corner; the wet path's 250 pF series cap into the pot is a high-frequency-weighted coupling, so its low-frequency corner depends on the pot setting and the TONE network and should be simulated rather than assumed.

## 3. Supply
- TR1 -> three diodes in series (half-wave) -> first filter 40 uF/450 V (node printed "+300 V") -> choke (series inductor) -> second filter 40 uF/450 V ("+295 V" B+). B+ node feeds the 6K6GT primary end and screen (+295 V directly).
- B+ -> 10K 1 W -> +250 V rail with 40 uF/450 V (preamp rail for 12AT7 plates, 7025 plates, dry-buffer bias divider).
- Printed voltages: +300 V (after first cap), +295 V (B+), +285 V (6K6 plate), +250 V (rail), 12AT7: +105 V (stage 1 plate), +120 V (stage 2 plate), cathodes +1.8 V / +2 V; 6K6GT cathode +24 V; 7025: +160 V plate (A), +1.2 V cathode, +250 V plate (B), +120 V grid (B), +130 V cathode (B).

## 4. Not legible / uncertain
- Reverb tank type and impedance; TR2 turns ratio and winding impedances (not printed).
- Recovery cathode capacitor: "6" uF with 150 V on the schematic versus 250 V on the layout (?).
- Whether the 10K between the junction node and the rail is 1 W (printed 10K 1 W) - and whether the choke has a value (not printed).
- Pilot lamp and heater winding details.
