# Fender Deluxe 5C3 - schematic read

Source: `3c780ca8-Fender_deluxe_5c3.pdf` (1 page, 1003x611 px raster, no text layer). Title "FENDER 'DELUXE' SCHEMATIC / MODEL 5C3". No drafting code, date or revision letter shown. No voltages printed anywhere. No layout page. Hand-lettered original Fender-style drawing.

Reading conventions used below: R in ohms (K = x1000, MEG = x1e6); caps are printed as "value-voltage" with the value in uF (".05-600" = 0.05 uF / 600 V; ".0005" = 500 pF; "25-25" = 25 uF / 25 V; "16-450" = 16 uF / 450 V).

## 1. Tube complement
| Role | Tube |
|---|---|
| Preamp V1 (two triodes, two channels) | 6SC7 |
| Phase inverter V2 | 6SC7 |
| Output (push-pull, cathode-biased) | 2 x 6V6GT |
| Rectifier | 5Y3GT |

## 2. Preamp stages (V1 = 6SC7, both triodes, cathodes tied together and grounded directly - no cathode R, no bypass)
Inputs: three jacks (top, middle, bottom), each with a shorting/ground spring drawn.
- **V1a (upper triode) - "channel 1", jacks 1+2:** jack 1 tip -> 7.5K -> common node; jack 2 tip -> 7.5K -> same node (a 7.5K+7.5K mixing/isolating pair). Common node -> .1-200 -> V1a grid. Grid leak 5 MEG to ground. Plate load 250K to B+ (see PS).
- **V1b (lower triode) - "channel 2", jack 3:** jack tip -> .1-200 -> V1b grid directly (no series R). Grid leak 5 MEG to ground. Plate load 250K to B+.
- Both 250K plate loads join at one point which goes to the B+ rail (shared node, same node as PI plate loads).
- Plate -> coupling cap .05-600 (each) -> its own volume pot.
- **Volume pots:** two, each 1 MEG (taper not printed). As drawn, the coupling cap line ends in an arrow on the pot body (wiper symbol). Upper pot: one end to ground, wiper = signal input, other end to the common "mix" node. Lower pot: one end to ground (bottom), other end (top) to the mix node, signal arrow lands on the pot between. Net effect as drawn: each pot is a wiper-driven shunt-type attenuator, outputs of both tied at the mix node (no separate mixing resistors). (?) wiring interpretation of arrow = wiper; compare Princeton/Champ drawings where the same arrow style is clearly the wiper.
- Mix node -> V2 PI grid 1 (no other grid resistor; DC return to ground via the two 1 MEG pots).

## 3. Tone control (one, "tone" on channel 1 signal only)
- Node at the output of V1a's coupling cap (the upper volume pot's input node) -> .0005 (500 pF) -> top end of a 1 MEG tone pot; pot bottom end -> .005 (5 nF, voltage not printed) -> ground; tone pot wiper -> mix node.
- Pot value 1 MEG, taper not printed. No other tone R/C.

## 4. Tremolo / vibrato
None.

## 5. Phase inverter V2 (6SC7, "self-balancing"/paraphase type)
- Triode 1 grid = mix node. Triode 2 grid is fed by a curved wire from the tap on the output-grid divider (see below). Both triodes share cathode: **2500 ohm in parallel with 25-25 (25 uF/25 V)** to ground.
- Plate loads: 250K each to the B+ rail node (3rd node, see PS). The two 250K loads are drawn as a tied pair at the B+ node.
- Triode 1 plate -> .05-600 -> upper 6V6 grid node. Triode 2 plate -> .05-600 -> lower 6V6 grid node.
- Output-grid divider (upper to lower): upper 6V6 grid node -> **1 MEG** -> tap (to PI triode 2 grid) -> **25K** -> centre node (grounded) -> **250K** -> lower 6V6 grid node. Also a **1 MEG feedback resistor** from the upper 6V6 grid node up to the OT secondary hot terminal (see 6). So the upper grid's DC leak is 1 MEG + 25K; the lower grid's is 250K. Tap fraction 25K/(1M+25K) (derived ~2.4%).

## 6. Output stage
- 2 x 6V6GT push-pull, **cathode (self) bias**: both cathodes tied -> **250 ohm 5 W** in parallel with **25-25** (25 uF/25 V) -> ground. Grid leaks as in 5. **No grid stopper resistors.** Screens wired straight to the screen B+ node (no screen R drawn). Plates to OT primary ends; OT centre tap to B+ node 1.
- Printed voltages: none. OT impedance: not printed. Speaker: one 8 ohm speaker jack plus "EXTRA 8 ohm SPKR" jack (the extra-speaker jack tip is tied to the secondary hot terminal).
- **Global NFB:** yes - 1 MEG from the secondary hot terminal to the upper 6V6 grid node (also the node at the .05-600 from PI triode 1). No presence control.

## 7. Power supply
- 5Y3GT: two HV windings ends to plates, HV centre tap grounded; 5 V winding to filament. B+ taken from the rectifier filament/cathode.
- Filter chain (derived from the drawing, rectifier -> preamp): [node 1: 16-450, OT centre tap] -> **10K** -> [node 2: 16-450, 6V6 screens] -> **10K** -> [node 3: 16-450, V1 plate loads 250K x2, PI plate loads 250K x2]. Both the V1 and V2 plate loads sit on node 3 (the V1 B+ feed is a long wire to the rail's left end, which is node 3).
- All three caps 16-450 (16 uF/450 V). No bleeder drawn, no choke.
- Primary: AC switch, **2 AMP fuse**, **.05-600** from the fused leg to ground (no ground switch). 6.3 V winding labelled "TO ALL 6.3 VOLT FILAMENTS" (no pilot lamp mentioned). No power transformer part numbers.
- Printed B+ voltages: none.

## 8. Not legible / uncertain
- Tone-cap voltage ratings and the two coupling cap voltage ratings are printed (.05-600) but the tone .005 / .0005 have no voltage.
- (?) Whether the arrow into each volume pot is the wiper (reading used) - see 2.
- (?) Whether the jack springs shown connect as normalling/shorting contacts: not determinable.
- Pot tapers, OT/PT part numbers, all DC voltages: not on this drawing.

## Changes between revisions (5C3 is the newest of the 5A3/5B3/5C3 trio in this pack)
- 5B3 -> 5C3: input isolating resistors 50K -> 7.5K, input coupling caps .05-600 -> .1-200, tone-pot ground cap .01 -> .005, filter caps 20-450 -> 16-450.
- 5B3 -> 5C3: output-grid/PI network 250K / 7000 / 250K -> 1 MEG / 25K / 250K, and a new 1 MEG global-NFB resistor (OT secondary -> upper 6V6 grid) plus an EXTRA 8 ohm speaker jack added.
- 5C3 has no layout/voltage page in this file (5B3 has printed voltages; they apply to the 5B3 values and should not be assumed to hold for 5C3).
