# Fender Deluxe 5A3 - schematic read (WARNING: composite/edited drawing)

Source: `32fd4944-fender_deluxe_5a3.pdf` (1 page, letter, a 1506x940 1-bit raster with a typed-text overlay). Page title: "FENDER 'DELUXE' SCHEMATIC, MODEL 5A3".

**Provenance caveat (important).** The embedded raster is the **5C3 drawing** (its own hand-lettered title reads "MODEL 5C3", tubes 6SC7/6SC7). On top of it, white patches and **typed Times New Roman labels** (a text layer: "5A3", "6SL7", "100pF?-1000V", "250K", "7.5 K", ".05-600" x2) were added by someone, and the 1 MEG NFB resistor and the EXTRA-speaker jack were whited out. So the 5A3 content = 5C3 art with edits. Values that are hand-lettered italic are the 5C3 original and were NOT re-checked against any real 5A3; only the typed labels are the editor's claims. Treat this file as a reconstruction. The "100pF?" carries the editor's own question mark.

Caps are "value-voltage" in uF (".05-600" = 0.05 uF/600 V; ".0005" = 500 pF; "25-25" = 25 uF/25 V; "16-450" = 16 uF/450 V).

## 1. Tube complement
Preamp V1: 6SC7. Phase inverter V2: **6SL7** (typed overlay; the underlying art says 6SC7). Output: 2 x 6V6GT. Rectifier: 5Y3GT.

## 2. Preamp (V1 6SC7, cathodes tied and grounded directly, no R/C)
- Upper triode (jacks 1+2): jack 1 tip -> 7.5K (hand-lettered) -> node <- 7.5K <- jack 2 tip; node -> **.05-600** (typed overlay; art said .1-200) -> grid; grid leak 5 MEG to ground; plate load 250K.
- Lower triode (jack 3): jack -> **.05-600** (typed) -> grid; 5 MEG to ground; plate load 250K.
- Plate loads (250K x2) tie to the B+ rail (lowest node).
- Plate -> .05-600 -> volume pot (2 x 1 MEG, taper not printed); same arrow-on-pot arrangement as 5C3 (wiper reading (?)); outputs tie at the mix node -> PI grid 1.

## 3. Tone control
As 5C3: node after upper triode's coupling cap -> .0005 -> 1 MEG pot -> .005 -> ground; wiper -> mix node.

## 4. Tremolo
None.

## 5. Phase inverter V2 (6SL7 per overlay; self-balancing, shared cathode)
- Cathode: 2500 ohm || 25-25 (25 uF/25 V).
- Plate loads 250K each to the B+ rail. Coupling caps .05-600 to each 6V6 grid.
- Output-grid network (typed): upper grid node -> **250K** -> tap (to PI grid 2) -> **7.5K** (typed "7.5 K") -> grounded centre -> **250K** -> lower grid node. (art said 1 MEG / 25K / 250K; typed overlay replaced the 1 MEG with 250K and the 25K with 7.5K). Tap fraction 7.5K/257.5K ~ 2.9% (derived).
- Typed addition: a capacitor drawn between the two PI plates, labelled "100pF?-1000V" (editor's own "?").

## 6. Output stage
- 2 x 6V6GT, cathode bias 250 ohm 5 W || 25-25 (25 uF/25 V). No grid stoppers, screens to screen node, no printed voltages. Single speaker (8 ohm not stated on the sheet here); the extra-speaker jack and the NFB resistor are removed on this sheet. OT impedance not printed. No NFB.

## 7. Power supply
5Y3GT; chain: [16-450, OT CT] -> 10K -> [16-450, screens] -> 10K -> [16-450, preamp/PI]. AC switch, 2 AMP fuse, .05-600 to ground. No voltages printed.

## 8. Not legible / uncertain
- Which labels are original 5A3 data versus editor's: unknowable from the file.
- "100pF?-1000V" (question mark in the source); wiper reading of volume arrows; all voltages absent; the underlying hand-lettered tone/ground cap values copied from 5C3 (".005", ".0005", "16-450").

## Changes between revisions (this file as a reconstruction of 5A3)
- 5A3 -> 5B3: input isolating R 7.5K (5C3 art) -> 50K; filter caps 16-450 -> 20-450; tone ground cap .005 -> .01.
- 5A3 -> 5B3: PI/output-grid divider 250K/7.5K/250K -> 250K/7000/250K (nearly identical ratio); both lack NFB and extra-speaker jack; 5A3 sheet adds a PI plate-to-plate cap (typed, "100pF?-1000V"); 5B3 shows it only in the layout socket.
- 5A3 vs 5C3 (same underlying art): PI tube 6SL7 vs 6SC7, input caps .05-600 vs .1-200, divider 250K/7.5K/250K vs 1M/25K/250K, NFB 1 MEG and extra-speaker jack absent in 5A3.
