# Batch 2 amp schematics versus the plugin (Mark I, Mark IV, Powerball, Savage 120, Sunn Model T / T Super, Dumble '97, Friedman BE-100)

Written from the seven reading files in this folder (`Mesa_MarkI.md`, `Mesa_MarkIV.md`, `Engl_Powerball.md`, `Engl_Savage120.md`, `Sunn_ModelT.md`, `Dumble_97.md`, `Friedman_BE100.md`) and the code and docs in `/mnt/user-data/outputs/StarterAmp_v0.1/` (`AMP_SOURCES.md`, `CIRCUITS.md`, `MODELED_GEAR.md`, `Source/`). Nothing in the code or the existing docs was changed. Line numbers are from `Source/PluginProcessor.cpp`, `Source/PowerSection.h` and `Source/ToneStackTmb.h` as they stand now.

How the code is organised, for reference (what the comparisons below refer to):
- Model index to name (from `AMP_SOURCES.md` and the `fam[]` table, `PowerSection.h` line 26): 2 Brown Eye, 4 Modern Maul, 7 Bumble Lead, 8 5555 Block, 11 Marque Dream, 12 Citrus Doom, 18 Wreck Stack, 19 Power Brawl, 27 Marque Five.
- Power-feedback family per model (`PowerSection.h` line 26, `loopGainFor` line 48): 2 Brown Eye -> 1 (British 50 W, 100k feedback, loop gain 0.79 / 1.71 / 2.96 for loose / normal / tight); 7 Bumble Lead -> 4 (light, 220k, 0.29 / 0.63 / 1.11); 11 Marque Dream, 18 Wreck Stack, 19 Power Brawl, 27 Marque Five -> 2 (British lead, 39k, 1.83 / 3.65 / 6.06); 12 Citrus Doom -> 3 (no feedback). Note `MODELED_GEAR.md` line 145 still says "Bumble Lead and Marque Dream: light feedback", but the array (index 11) has Marque Dream at family 2, consistent with the later v90 note that moved it.
- Component voicings (first-stage cathode and coupling corners, bright cap) live in `applyComponentVoicing` (`PluginProcessor.cpp` lines 1566 to 1655). The only models with entries relevant here: 2 (shares the British 50 W entry, line 1646), 7 (line 1585), 11 (line 1580), 27 (line 1634). There is no entry for 12, 18 or 19.
- Passive stacks are a user-selectable menu (`ampStack`, `stackSel`, `PluginProcessor.cpp` lines 7996 to 8009; `ToneStackTmb.h` `valuesFor` types 0 to 12). They are not auto-assigned per amp.
- The only full-chain "solved from schematic" model is Lead 50 (JCM800 2203, `MODELED_GEAR.md` line 105). None of the eight amps below has one. Stage curves are generic per amp (`MODELED_GEAR.md` "Stage model (v80)").

---

## 1. Mesa/Boogie Mark I (`Mesa_MarkI.md`)

Maps to: Marque Dream (11), the third "Vintage" voicing (`PluginProcessor.cpp` line 567 comment: "early-Mark-style sound, lower gain, thick creamy mids, smaller sweeter low end"; `MODELED_GEAR.md` line 32 and line 65). No model-specific Mark I component voicing exists (line 1580 applies one entry to all Marque Dream voicings, read from the Mark IIC+/IV rhythm channel). No solved model.

Agreements
- Stack: 250k treble, 250k bass, 10k mid, 100k slope, 250 pF / 0.1 uF / 0.047 uF is exactly `valuesFor` type 0 (`ToneStackTmb.h` line 24), which the code already treats as the American-combo/Mark stack. My solver at 38k source gives noon loss about -18 dB (mean) and -18.6 dB at 1 kHz with a notch near 400 Hz.
- Supply B+: 449 V at the OT center tap (printed) versus `supplyFor` case 11 448 V (`PowerSection.h` line 289). Agrees.
- Four 6L6 power tubes, 100 W class.

Disagreements and new information
- Cathode bypass: Mark I V1 and V2 cathodes are 1.5K (the note "1.5K 1/2 W resistors") with 22 uF/15 V bypasses (plus a 1N5303 current-regulator diode noted, arrangement unclear). The corner is 1/(2 pi 1.5K 22 uF) = about 4.8 Hz, effectively fully bypassed. Marque Dream's code line 1580 uses 1.6k / 0.47 uF (corner about 226 Hz, from the later Marks). So for the Vintage voicing the first-stage low-cut is about 45 times too high in the code. This is the clearest concrete difference for a Mark I character (fuller, less low-cut).
- Power amp grid stoppers and screens: 2200 ohm on pin 5 (control grid) and 470 ohm 1 W on pin 4 (screen), so `CIRCUITS.md` (line ~199, "four output tubes with 470 ohm grid stoppers and 2.2k resistors") has them reversed. Not used in code.
- The Mark I has a series-cascade two-volume structure (Volume 1, Volume 2 with a 110 pF bright on Volume 2) and a footswitch Lead Boost through the mid pot. The current Vintage voicing is described only qualitatively.
- No clipper diodes; the optional five-band graphic EQ (80/240/750/2200 Hz and a cut/boost) is not a part of any current model.

New model or voicing worthwhile? A voicing override, not a new amp: Vintage should use the Mark I cathode corners (hp[0] near the 30 Hz floor) and the 110 pF bright on the second volume. Small change in `applyComponentVoicing` keyed on the voicing index. Moderate value.

---

## 2. Mesa/Boogie Mark IV (`Mesa_MarkIV.md`)

Maps to: Marque Dream (11) (Rhythm and Lead voicings) and Marque Five (27, partly fitted to Mark IV captures: `MODELED_GEAR.md` line 217). `AMP_SOURCES.md` lists the Mark IV as "received, only the bright stack applied"; `CIRCUITS.md` lines ~203 to 206 record it. No solved model.

Agreements
- Stack type 11 (`ToneStackTmb.h` line 34: 250k / 250k / 10k / 100k / 1000 pF / 0.1 uF / 0.047 uF / 44k source) matches the schematic exactly in all component values (R201 150K plate load with about 62K rp gives about 44K source), but it represents the Fat-switch state (C12 250 pF plus C32 750 pF = 1000 pF). The standard state (250 pF) is type 0 with a 44k source. My solver: noon loss at 1 kHz -19.0 dB (250 pF) versus -11.5 dB (1000 pF), a 7.5 dB level difference and the notch moves from about 400 Hz to about 200 Hz. A user choosing the "Mark IV" stack gets the Fat state only.
- Reading in `MODELED_GEAR.md` line 163: V3A 82K plate (R282 82K, agrees), 2.2 uF cathode cap (agrees); V3B 270K plate (R276 270K, agrees), 0.047 uF output coupling (C236, agrees), 22K switched in with 0.22 uF (R226 22K / C311 0.22 uF, agrees), lead-drive pot 1M (agrees), reverb stages 12AT7 (agrees).
- First-stage cathode 1.5K with 0.47 uF (R251 / C321) versus code 1.6k / 0.47 uF (line 1580): within 7 percent, no audible difference.
- Supply: 448 V at B, 460 V at A, 385 V and 370 or 378 V further down: `supplyFor` case 11 (448 V, 5.6k x 30 uF) agrees; the 5.6K/2 W and 30 uF/500 V are as printed.
- Feedback: R104 56K with C211 0.005 uF across it is exactly the network quoted in the Mark V note (`MODELED_GEAR.md` line 172: "56k in parallel with 0.005 uF").

Disagreements and new information
- Reading discrepancy: the doc says V3B plate "270k with 600 pF"; the schematic prints C23 500 pF (page 4). Possibly a 5/6 misread either way; low impact.
- The doc and code comment mention a "220 uF reservoir". The schematic has two 220 uF/300 V in series with 150K/1W balancing resistors (about 110 uF, 600 V). Reservoir size affects sag: `supplyFor` 11 has rs 60 ohm and a 0.185 s slow tau (the slow tau is the 5.6k x 33 uF figure, fine) but if the fast-stage model uses reservoir capacitance anywhere, 110 uF is the right number.
- V3A to V3B coupling: R275 270K / C222 0.02 uF into R105 68K with C4 1 nF across: this is a -14 dB divider with a 1 nF shunt (3 kHz corner), which behaves like a cold clipper-style attenuator between lead stages. The engine's single "lead" chain does not model it (the doc itself says "the lead channel does not map to one of the three stages in my chain").
- Power amp: Simul-Class with V6/V7 (cathodes switched) and V8/V9 (bias -54.5 V from the 2.2M/220K divider on a -60 V rail, TRI/PENT screen switch, 2.7K/2 W screens versus 470/2 W on V6/V7). The code assumes one conventional stage per amp. No model of Class A mode exists (Marque Dream's `PowerSection` family 2 is a single push-pull pair).
- Feedback: the presence network has three switched pots (R1, R2, LD 250K each) with 680 pF in the R2 leg and 0.001 uF in the LD leg. My estimate for the fraction at the injection node is about -41 dB at presence 0 and -35 dB at 1 kHz, much lighter than family 2 (39k). This estimate depends on a node assignment I marked (?), so treat as an indication only: the Mark IV may be closer to family 4 (light, 220k) than family 2.
- The 22 LDR switching matrix (page 9 legend) is not modelled; mode differences (R2 low boost, R2 Fat, Lead Fat) are not in the code.

New model or voicing worthwhile? Not a new amp (Marque Dream and Marque Five already cover the Mark IV via captures). Two modest things: add a type 11b stack with the 250 pF (standard) state next to the Fat one, and decide whether the Simul-Class power amp deserves a "Class A" switch on Marque Dream (a power-section feature, larger effort, uncertain value).

---

## 3. ENGL Powerball (`Engl_Powerball.md`)

Maps to: Power Brawl (19), `MODELED_GEAR.md` line 37: "Built from the owner's manual... The schematic scan was too small to read values from." Supply case 19 (`PowerSection.h` line 298) is read from "440 V on / 460 V standby printed, 470R screen resistors, 220 uF reservoir". No component voicing entry for model 19. Family 2 (British lead).

Agreements
- 440 V (play) / 460 V (standby) printed; CE8 220 uF/500 V reservoir with CE9 47 uF/500 V; R72 to R75 470R* 1.4 W screens; four 6L6GC; 2.2K grid stoppers; this all matches the comment.
- The cathode and plate readings: first stage 100K plate, 1K / 22 uF cathode; V5B 1.5K / 22 uF.

New information (the sheet is readable at crop level, so the "too small" statement is out of date)
- Stack values: clean/crunch stack 250KB treble (separate Clean and Crunch pots), 1MA bass, 20KB middle, 100K slope (R27/R31), 470 pF (940 pF with Style), 220 nF bass cap, 47 nF mid cap (Style selects 100 nF / 47 nF caps); lead stack the same with 22 nF mid caps (open middle) or 47 nF (focused middle). Solved: Style off noon loss about -15.3 dB at 1 kHz, notch near 280 Hz; Style on about -12.3 dB, notch near 200 Hz; lead stack -15.7 dB / 410 Hz (open) or -12.4 dB / 290 Hz (Style). Source (about 40k) and load (about 300k) are assumptions. None of these is in the `valuesFor` menu (type 5 to 7 are the nearest, 680 pF 20 nF, but with 47.5k slope and different pots).
- Preamp couplings: 47 nF into 100K-class grid networks everywhere, 100 nF at V6B/V7B: first-stage cathode corners 7 Hz (1K / 22 uF) and 48 Hz on V6A (3.3K / 1 uF).
- Power amp: feedback via R59 150K into the PI cathode node (R56 22K / R57 4.7K): a first-order estimate of the loop attenuation of about -18 dB at best. For comparison, family 2's 39k and family 1's 100k feedback resistors against a typical cathode resistor give roughly -11 and -18 dB; the estimate sits closer to family 1 (British 50 W, loop gain 1.71 normal) than to family 2 (3.65). The node assignment is marked (?), so this is only an indicator that the Power Brawl may be fitted too tight.
- Two output pairs with grounded cathodes (fixed bias -55 V), 4.7R per-tube cathode sense resistors, bias 28 to 34 mA per tube.

New model or voicing worthwhile? Yes as a voicing refresh of Power Brawl (add a component entry for model 19 and two stack menu entries: "Powerball clean/crunch, Style on/off" and "Powerball lead"). The Style switch (940 pF treble cap and different mid cap) is a real, readable difference and maps to an existing "Bottom/Middle" idea in the voicing names. No new amp.

---

## 4. ENGL Savage 120 (`Engl_Savage120.md`)

Maps to: Wreck Stack (18), `MODELED_GEAR.md` line 36 ("Read from the Savage 120 schematic: supply rails, cathode networks, feedback and presence network"). Supply case 18 (`PowerSection.h` line 297): 545 V plates, 335 V screens, 2 x 220 uF reservoirs, 150k and 330k dropping resistors. Family 2. No component voicing entry for model 18 (the cathode networks were "read" but are used only through the stage curves).

Agreements
- Everything in the supply comment matches: UA 545 V, US.G 335 V (via 470 ohm 5 W), two 220 uF reservoirs (220 uF/250 V and 220 uF/450 V), 150k and 330k 1.1 W resistors.
- A single pair of 6550 (V7, V8), not four (the code comment says "6550 pair"). 1K screen resistors? The sheet prints 1K / 1 W-class resistors from +U S.G to pin 4 of each 6550.
- Cathode networks 1K5 / 22 uF and 1K / 22 uF; plates 100K; 47 nF couplings.

New information
- Feedback: R150K from the OT into the V6B grid with 22K to ground; presence 50K pots with 4n7 (A and B, relay-selected). First-order estimate: 22K / (22K + 150K + 4.7K) = about -18 dB at best. Closer to family 1 (British 50 W) than family 2 (loop gain 3.65) by the same reasoning as Powerball, but with the node caveat (?).
- Only five gain stages in the lead path (V1B, V2A, V2B, V3A, V3B), with V2B plate 330K and 3K3 unbypassed cathode on that stage: the voicing's "first-stage cathodes" reading can be extended to V2B (330K plate, 3K3 unbypassed, so a gain-limited stage that behaves differently from the others).
- Stack caps are not legible, so no menu entry can be derived.
- Bias adjustment 0.8 to 1.2 V across the cathode measuring resistor; bias supply -U GI not printed.

New model or voicing worthwhile? No new model. Possibly check the feedback family (low priority).

---

## 5. Sunn Model T and Model T Super (`Sunn_ModelT.md`)

Maps to: Citrus Doom (12), "Bass stack" voicing (`MODELED_GEAR.md` lines 33 and 66, "Ampeg SVT / Sunn Model T territory"); `AMP_SOURCES.md`: "Sunn Model T: only the stack is applied." Family 3 (no feedback) via `fam[12]`; supply case 12 (`PowerSection.h` line 294): 500 V, 0.50 A, 110 ohm, 0.60 s, 0.30, all estimated. No solved model. The Model T Super has no mapping and is not documented elsewhere.

Agreements
- Stack: `valuesFor` type 10 (`ToneStackTmb.h` line 33: 250k / 1M / 25k / 56k / 270 pF / 22 nF / 22 nF / 1k source / 1M load, linear pots) matches the 1973 sheet component for component, including the pot tapers (printed note 4: R15, R16, R18 linear). My solver: noon loss -9.4 dB mean, -12.5 dB at 1 kHz, notch near 660 Hz. Very low insertion loss compared with a Marshall stack.
- Supply: 519 V no load / 481 V at 150 W (A), 419 / 390 V (B), 351 / 330 V (C). The code has 500 V.

Disagreements and new information
- Feedback: the Model T has real global feedback from the 16 ohm tap through R20 22K into R26 10K to ground and C10 0.1 uF into the PI (V3B grid); the flat attenuation is 10K/32K, about -10 dB, with presence R25 25K and C9 0.022 uF shunting it at HF. Family 3 ("no feedback", `PowerSection.h` line 26 index 12) therefore does not match the amp. By the same reasoning as above, -10 dB is heavier than family 2 (about -11 dB) so the real amp is at least a "British lead" family, but its 5 to 15 percent damping is the sort of thing the Sunn is known for. (Estimate with an ideal node; the exact PI cathode wiring is marked (?).)
- Output stage: four 6550 with 47 ohm / 5 W plate resistors (Model T) or 50 ohm (Super), screens fed through 1.5K / 5 W from output-transformer taps (ultralinear-style, with screens at 504 V against plates at 507 V), grid stoppers 1K, fixed bias -55 V. `CIRCUITS.md` (lines ~188 to 194) says "470 ohm/5 W cathode resistors"; the sheet prints 47 ohm / 5 W (the "0" is the ohm sign) in series with the plates (not cathodes), and screens connect to taps, not to B+ ("1.5k/5 W screen resistors" is right).
- Supply droop data to calibrate `supplyFor` 12: rail A falls 519 -> 481 V (7.3 percent), B 419 -> 390 V (6.9 percent), C 351 -> 330 V (6.0 percent) from no signal to 150 W RMS into 4 ohm. The code's depth is 5 Ipk Rs / B+ = 5 x 0.5 x 110 / 500 = 0.55 (55 percent!), which looks very large compared with a 7 percent measured sag at full power (the formula is a peak, per-note estimate, so only the order of the number is a concern). Worth rechecking the "Wall" and "Bass stack" voicings against this amp's stiff supply (two chokes, 20 uF caps, silicon diodes).
- The Model T's signal path is a clean, high-headroom path: two parallel 12AX7 triode stages (brite / normal, mixed by 470K resistors), one 12AX7 gain stage, a cathode follower and the stack, then the long-tail PI; no gain beyond that. It will not clip before the power section, unlike Orange-style Citrus Doom "Stoner".
- Model T Super: 1M treble pot, 1M bass, 25k reverse-log mid with a three-position mid network (capacitors C23 to C25 and an inductor L1, values not legible), an additional gain stage (V2B with 120K plate) and 22K presence; the stack could not be derived.

New model or voicing worthwhile? The existing Bass stack voicing could be improved without a new amp: switch family from "none" to a feedback family and apply the clean stack and the supply data. A Model T as its own amp is not justified (it is a headroom amp).

---

## 6. Dumble '97 (`Dumble_97.md`)

Maps to: Bumble Lead (7), whose voicing (line 1585) is "read from the '70s and '97 Dumble-style sheets" (`AMP_SOURCES.md` row 7: "cathodes, bright cap, OD coupling, supply"). Supply case 7 (`PowerSection.h` line 292) reads the '70s sheet (436 V, 110 ohm, six diodes, 100 uF pair). Family 4 (light feedback).

Agreements
- 500 pF bright cap on the 1M volume pot? The '97 prints 390 pF in parallel with 220K across the volume pot top (a treble bleed); the code comment's 500 pF comes from the '70s sheet. Not the same value; line 1585 describes both sheets as the source.
- 12AX7 preamp tubes, EL34 pair, 4k7 feedback resistor and 2kB presence with 390R / 1 uF (as already recorded in `CIRCUITS.md` lines ~87 to 99). My reading confirms those numbers exactly.

Disagreements and new information
- Cathodes: the '97 uses 3k3 / 5.0 uF (V1B, V1A), 2k7 / 5.0 uF (OD first triode) and 1k8 / 1.0 uF (second OD triode). The code's hp[0] and hp[1] are both 1.5k / 5 uF from the '70s, floored at 30 Hz (`jmax (30.0f, ...)`): corners with 3k3 / 5 uF are 9.6 Hz (below the floor, no change). The OD stages are different: 1.8k / 1 uF is an 88 Hz corner and the code's hp[2] is the coupling only.
- OD coupling: the '97 couples the first OD triode into the 250kA Drive pot via 0.0022 uF into 100K (about 720 Hz high-pass with only the 100K, about 1 kHz with the pot's track in parallel). The code (line 1590) uses "0.02 uF into the 100k overdrive pot" (about 80 Hz) from the '70s sheet. That is a factor of nine in corner frequency. Because the '97 sheet's red/blue legend (confirmed / probable) is lost in the supplied grayscale, I cannot say whether 0.0022 uF is a confirmed value. Flag for the user: it would make the '97's OD channel much brighter and tighter than the current OD behaviour, if confirmed.
- Stacks: the clean stack (330 pF, 150k slope, 0.1 uF, 0.01 uF, 1 nF across the bass pot, 500kA bass, 250kA mid as a resistor, 9k8 to ground) differs from type 8 (`ToneStackTmb.h` line 31: 250k bass, 100k mid, 100k slope, 0.1 / 0.05 uF, 1.8k under the bass pot). Insertion loss: mine -15.0 dB at 1 kHz, notch near 420 Hz. The OD stack (330 pF, 33k slope, 22 nF, 22 nF, 250kB / 1MB / 20kB) is close to type 1 (JCM800 style, 33k slope, 470 pF) but with 330 pF and a 20k mid: -16.3 dB at 1 kHz, notch near 780 Hz (mean -12.5 dB). Neither '97 stack is a menu entry.
- Feedback: 4k7 from the speaker into the PI input rail, shunted by 390R + the 2kB presence pot (as a rheostat) + 1 uF to ground. The fraction at the node (ideal) is about -22 dB at HF with the pot at 0 up to about -9 dB with the pot at 2k; at 100 Hz about -12 dB because the 1 uF is not a short there. That is considerably more feedback than family 4's 220k resistor would imply (loop gain 0.63 at normal feel), so the '97's power section is probably closer to families 1 or 2 than 4 (estimate; the preamp source impedance at the node is ignored).
- Supply: 465 / 458 / 433 / 361 / 323 V rails, 200 uF/300 V x2 with 330k balancing, 47 uF x2 networks, 3k / 12k / 10k droppers; `supplyFor` 7 uses 436 V from the '70s sheet (the '97's B+3 at 433 V, plate rail at 458 V). Same order.
- Preamp gain structure: two preamp channels (clean: two stages; OD: two more stages), channel selection by a DPDT; no clippers.

New model or voicing worthwhile? Yes, modest: add two stack menu entries (Dumble '97 clean stack and Dumble '97 OD stack, with the clean one needing the extra 1 nF across the bass pot, which `valuesFor` supports only via the existing Rn/extra-cap fields (type 8 has `Rn = 1800` and no Cmn), so the menu code would need one new field) and decide whether to raise the OD coupling corner.

---

## 7. Friedman BE-100 (`Friedman_BE100.md`)

Maps to: Brown Eye (2) (`AMP_SOURCES.md` row 2: "Refit in b26 from the capture sets plus the BE-100 schematic"). Voicing uses the shared British 50 W entry (line 1646: 2.7k / 0.68 uF first stage, 0.022 uF into 470k) and the metal controls (line 1463: Tight goes to 2.5 octaves for model 2). Family 1 (British 50 W: 100k feedback, loop gain 0.79 / 1.71 / 2.96), supply case 2 (420 V, 0.30 A, 100 ohm, 10k x 50 uF). Not a solved model; `MODELED_GEAR.md` line 105 lists the Friedman BE-100 as "received but not yet modeled".

Agreements
- First stage: V1A 2.7K / 0.68 uF (R4, C5) and V2A 2.7K / 0.68 uF are exactly the numbers in the 50 W British entry (corner 87 Hz).
- Tone stack: 500 pF, 33K, 22 nF, 22 nF, 250kB treble, 1MA bass, 25kB mid is JCM800-class; type 1 (470 pF, 33k) is within 6 percent. My solver gives -8.1 dB at 1 kHz (JCM800 reference -8.3 dB).
- Tight / Fat knob in code (line 1461): "coupling network runs from a Fat setting (low-cut near 100 Hz) to the tightest one (near 1 kHz), about ten times". My read of the network: Fat adds 22 nF in parallel with 2.2 nF (24.2 nF total): with 68K+68K 48 Hz or with 560K+220K 8.4 Hz; without Fat, 530 Hz (68K+68K) or 93 Hz (560K+220K). So the range is wider than ten times (about 8 Hz to 530 Hz) depending on S2 position, and the code's "near 100 Hz ... near 1 kHz" is one octave higher than my corner math. S2/SW8 pairing is uncertain (?).
- Gain range: the schematic's gain pot VR1 1M is before V2B; both preceded by -23.9 dB (1M + 68K) after V1A. The code's low gain multipliers (0.52 to 0.92) are consistent with a clean-ish lowest setting.

Disagreements and new information
- Couplings other than the first stage: the code gives hp[1] and hp[2] as 0.022 uF into 470k (15 Hz), JCM800 values. The BE-100 has 2.2 nF into 1M + 68K (68 Hz) after V1A and 2.2 nF with the switched network after V2A (see above), then 2.2 nF into the 1M gain pot network (R30 470K, R28 470K) for V2B. Those are all 10 times smaller capacitances: the code's base corners are about 5 to 35 times too low before Tight is applied. The Tight factor (about 5.6) hides part of this but the Fat end is not reached.
- Plate networks: V1A, V2A plate loads are 220K in series with 100K and a 500 pF cap across the 100K (HF compensation: the load drops from 320K to 220K above about 1.4 kHz at the plate, a treble roll-off or boost depending on viewpoint); V3A has 500 pF across the 100K plate load as well. These appear nowhere in the code.
- SAT switch: R33 10K, C25 100 nF and a pair of MPSA06 transistors as a clipper after the V3B cathode follower. The code has no hard clipper stage in Brown Eye. It could be a real part of the "Brown/Heavy" voicings' feel.
- Feedback: R50 47K with (R51 220K parallel 4.7 nF) from the transformer into the PI cathode network (R40 10K to ground, 5kB presence with 0.1 uF). Estimate: about -29 dB at low frequencies, about -16 dB at high frequencies (node assumed ideal). `CIRCUITS.md` says about 1:5 (-15 dB); my number agrees with that only at high frequencies. The code uses family 1 (100k feedback), which is about -18 dB, so family 1 is consistent with the high-frequency end, and lighter at low frequencies. Worth a check on the low end. The exact injection node is marked (?).
- Four EL34 with 1.5K stoppers, 1K / 7.5 W screens, fixed bias (supply -V from a UF4007 chain, P12 25K). Supply voltages are not printed.

New model or voicing worthwhile? Not a new amp. A set of corrected coupling values and the SAT clipper for Brown Eye.

---

## Ranked follow-ups (most valuable first)

1. Brown Eye (BE-100): replace the shared 50 W British couplings with the BE-100 network (2.2 nF into 1M + 68K = 68 Hz; V2A 2.2 nF with the switched 560K/68K and 220K/68K networks; Fat 24.2 nF), add the plate-load 500 pF compensation on V1A, V2A and V3A, and review the Tight knob range (it currently starts from a 15 Hz base corner that should be about 50 to 500 Hz). Biggest evidence, direct component match elsewhere.
2. Marque Dream "Vintage": give the third voicing a Mark I component entry (1.5K / 22 uF cathodes = 4.8 Hz corner instead of the 226 Hz of the IIC+/IV 1.6k / 0.47 uF; 110 pF bright on the second volume), and fix `CIRCUITS.md`'s swapped stopper/screen values for the Mark I power tubes.
3. Bumble Lead OD channel: check the OD coupling (0.02 uF from the '70s sheet versus 0.0022 uF in the '97, a 9x difference in corner) with the user, and add the '97 clean and OD stacks as menu entries.
4. Power Brawl (Powerball II): the sheet is now readable; add a component voicing entry (cathodes 1K/22 uF and 1.5K/22 uF, 3.3K/1 uF on V6A, 47 nF couplings), two Style-aware stack menu entries, and re-check the feedback family (my estimate is about -18 dB, closer to family 1 than family 2).
5. Citrus Doom Bass stack (Sunn Model T): family 3 ("no feedback") contradicts the printed -10 dB feedback network; apply the real UL-screen four-6550 stage with 47 ohm plate resistors and use the printed droop (519 -> 481 V at 150 W) to calibrate `supplyFor` 12.
6. Feedback families in general: my first-order -18 dB estimates for Powerball and Savage and -22 to -9 dB for the Dumble '97 are quite different from the families assigned (2 and 4). A single review of beta values for these three against the schematic nodes (needing exact PI cathode topologies, partly unreadable here) would be efficient.
7. Mark IV: add a standard (250 pF) variant of stack type 11 and correct the reservoir note (two 220 uF/300 V in series, about 110 uF); the 600 pF versus 500 pF reading of C23 in `MODELED_GEAR.md` line 163 should be 500 pF.
8. Model T Super stack and the Mark IV Simul-Class / Class A power amp: only if there is a clearer copy (the Super's stack parts are illegible; the Class A switch polarity is (?)).
