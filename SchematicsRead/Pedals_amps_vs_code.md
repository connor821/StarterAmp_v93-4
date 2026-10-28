# Where the three new schematics agree, disagree or add detail versus the code docs

Compared against: `CIRCUITS.md`, `MODELED_GEAR.md`, `AMP_SOURCES.md` (and, for the exact DC30 assumed value, one line of `Source/PluginProcessor.cpp`, read only). Nothing in code or existing docs was changed. Companion notes: `MetalZone_MT2.md`, `DOD250.md`, `MatchlessDC30.md` in this folder.

## A. Matchless DC30 ("Dual Thirty", amp 28)

### A1. The one assumed value, exactly

- **Assumed:** the **first-stage cathode resistor of Channel 1 = 1.5k, with a 25 uF bypass**. Where it is written: `Source/PluginProcessor.cpp` line 1641 ("first stage 220k plate with a bypassed cathode (value not legible, 1.5k assumed, 25 uF)"), and the line `hp[0] = max(25 Hz, bypassCornerHz(1500, 25e-6, 1600))`. The docs say the same in words: `AMP_SOURCES.md` row 28 ("first-stage cathode value unreadable, assumed"), `MODELED_GEAR.md` "Dual Thirty (amp 28)" ("Estimates: first-stage cathode value ..."), `AMP_BUILD_STATUS_b62.md` rows 28 and the ranked list ("no cathode value", "one value assumed"). The 25 uF itself is recorded as read (CIRCUITS.md line 132: "220k plate, 25 uF bypass").
- **What the schematic I was given shows:** nothing about it. The DC30 page supplied (`matchless_dc30-1.png`) is the **phase-inverter and power-amp sheet only** (headed "P/I - Power Amp"). It has no Channel 1 preamp, so the first-stage cathode resistor is **not on it** and cannot be settled from it. (If the unreadable page was the Channel 1 preamp sheet, it is a different image from this one.)
- **Side note on how much it matters in code:** with 1.5k and 25 uF the computed bypass corner is about 5.9 Hz, which the code then clamps up to 25 Hz (`jmax(25, ...)`). So within the current formula the assumed 1.5k does not change the audible bass corner at all; it would only matter if the cathode value were also used for stage bias or gain, which I did not trace.
- The sheet does give **other cathode-network values for the amp** (the power-stage cathode and the inverter tail), listed in A2.

### A2. What the DC30 sheet confirms from the code docs (CIRCUITS.md line 132)

| Doc statement | Sheet |
|---|---|
| long-tail 12AX7, 100k plates | confirmed: 100K plate loads to B+ node 3 |
| cut 250k / .0022 | confirmed: "CUT 250KA" pot in series with ".0022" cap (cap value could read .022 because the handwriting is rotated, flagged in the note) |
| master 1MA | confirmed: "1MA Master" |
| 4xEL84 cathode-biased | confirmed |
| 1k5 grid stoppers, 100 ohm, 220k | confirmed: 1K5 stopper per grid, 100 ohm per screen, 220K + 220K grid leaks |
| 62 / 68 ohm cathode | confirmed: **62 ohm 10 W** and **68 ohm 10 W** |
| Hi/Lo power switch | confirmed: "HI / LO power" switch |

### A3. What the sheet adds (not in the docs)

1. **Shared-cathode network detail.** HI: all four tubes, through the 62 ohm only. LO: the Y group (tubes 1 and 4) is left open, the X group (tubes 2 and 3) returns through 68 + 62 = 130 ohm. **250 uF / 100 V** bypass on the common cathode node in both positions. The docs give only "62/68 ohm" and "Hi/Lo power switch", not the bypass cap, the open Y group, or which resistor is in which position. (Reading of the switch contacts is medium confidence.)
2. **Inverter bias values:** both grids and cathodes are referenced to a node that goes through **47K to ground**, the cathodes join through **1K2**, the grid leaks are **1M + 1M**. The docs name only the plate loads.
3. **Coupling values:** CH1 and CH2 enter the two grids through **.01 uF / 400 V** caps; the plates couple to the power-tube grids through **.047 uF / 400 V** caps. Not in the docs.
4. **Where Cut and Master sit:** both are connected **between the two phase-inverter outputs**, after the .047 caps and before the 1K5 stoppers. Cut is a pot-and-cap RC shunt across the pair; Master is a 1 M bridge that a pull switch ("Pull MID/MV to close") connects. The docs call Master a "master 1MA" with no placement or switch.
5. **Output transformer:** printed "O/T WTI 9356", taps 04 / 08 / 16 (Z SEL), a Phase/Reverse switch. Nothing in the docs.
6. **B+ nodes** are drawn as circled 1 (output-transformer centre tap), 2 (EL84 screens, via 100 ohm each) and 3 (inverter plates); **no voltages are printed**. The docs' 350 / 338 / 308 / 317 / 319 / 320 V come from a different (supply) sheet, which I was not given.

### A4. Where the sheet and the code docs pull apart

- **Cut implementation.** `MODELED_GEAR.md` b39 treats the Cut as a **2.8 kHz treble shelf (+5.6 dB to -8.4 dB)** and says the values "are chosen to match the control's range, not fitted to a measurement". The sheet shows the real Cut as a **series 250k pot + 2.2 nF** bridging the two inverter outputs. The same shelf is used for both Chime Thirty (AC30) and Dual Thirty. But `CIRCUITS.md` line 37 records the AC30's Cut as a "250k log pot with **4.7 nF** across the power-tube grids". So the DC30 cap (2.2 nF) is **about half** the AC30 value (4.7 nF); for a similar source impedance the DC30 roll-off would sit at roughly **twice the frequency** of the AC30's, so one shared 2.8 kHz corner is probably wrong for one of them. (Indication only; the inverter source impedance was not calculated.)
- **Cathode follower (56k)** in the docs' Ch1 chain: **no cathode follower exists on this sheet**; it must come from the preamp sheet, which I cannot check.
- **Channel naming:** the docs and the plugin menu say "Channel 1, Channel 2, Pushed, Cut". The sheet's inputs are CH1 and CH2, so those match; there is **no Normal / Top Boost / Vibrato** structure on it. If the request wording referred to Normal/Top Boost/Vibrato, that is a Vox-AC30-style layout, not this sheet.
- **Rectifier ("SAR4/SV4", 33 uF, choke):** not on this sheet; unverified here.

## B. DOD 250 Overdrive Preamp (not in the docs)

There is **no DOD 250 entry** in CIRCUITS.md, MODELED_GEAR.md or AMP_SOURCES.md. The nearest existing item is **MXR Distortion+** (`CIRCUITS.md`: "Partly"; `MODELED_GEAR.md` v91/v93: "Gain 1 + 1M / (4.7k + pot), 47 nF, 10k / 1 nF into germanium diodes ... Estimated: 1 MHz bandwidth, 3.5 V swing"; Drive: Op-Amp Distortion). The DOD 250 build doc says it is "almost identical" to the Distortion+.

| Item | Code docs (Distortion+) | DOD 250 sheet |
|---|---|---|
| Gain formula | 1 + 1M / (4.7k + pot) | **same:** 1 + 1M (R8) / (4.7k (R7) + 500k pot), 2.98x to 213x |
| Cap in the gain leg | "47 nF" | **4.7 nF** (C3) |
| Clipper | 10k / 1 nF into **germanium** diodes | 10k (R9) / 1 nF (C6) into **1N4148 silicon, asymmetric: one diode one way, two in series the other** (about +0.6 V / -1.2 V) |
| Op-amp | "1 MHz bandwidth" (assumed) | **LM741** (GBW about 1 MHz at unity, so the closed-loop bandwidth at gain 213 is only about 4.7 kHz) |
| Swing | "3.5 V" (estimated) | not printed; the clipper sets the swing at roughly 0.6 V / 1.2 V, then a volume pot |
| Pot | not stated | gain **A500K** (original B500K reverse-log), volume **A100K** |
| Extras | not stated | input 10 nF + 10k + 2.2 M load, 22 pF and 1 M in the feedback, 4.7 uF output cap, 100 ohm / 22k / 22k / 10 uF supply and bias |

Differences worth checking in the code: **4.7 nF versus 47 nF** (a ten-fold shift in the leg corner: 7.2 kHz at minimum resistance versus about 720 Hz); and **silicon asymmetric versus germanium** clipping. The docs' "47 nF" may be correct for the MXR but wrong for the DOD 250, or may be a transcription slip; the DOD 250 sheet says 4.7 nF. The "Clipping" menu in the Distortion style has an "Asymmetric" option, but I found no description in the docs of what it is, so I cannot say whether it matches 1 + 2 diodes.

Also relevant to the Distortion style (**Boss DS-1**, `CIRCUITS.md`): the DS-1 clipper is documented as 2.2k / 10 nF into 1N4148. The DOD 250 clipper uses 10k and 1 nF, so the two are different networks even though the diode is the same.

## C. Boss MT-2 Metal Zone (not in the docs)

There is **no MT-2 entry** in any of the three docs. The closest existing items are Fuzz: Buzzsaw (HM-2), the "Metal Pedal" fuzz style (a merged guess from product descriptions; `MODELED_GEAR.md` v97) and Tight (Precision Drive-style); none uses MT-2 values. So all the following is new detail the schematic adds:

- **Topology:** JFET buffer, two gain stages, then a three-band tone section (HIGH/LOW, MIDDLE level, MID FREQ), then level; electronic bypass.
- **Op-amps:** four M5218AL duals, 8 sections.
- **Clipping:** exactly **two 1SS133 silicon diodes at one node after the DIST stage**, behind a **2.2k** resistor, no shunt capacitor drawn there. That is the same 2.2k as the DS-1 (`CIRCUITS.md`: 2.2k / 10 nF into 1N4148) but without the 10 nF. The diodes' orientation could not be read.
- **DIST stage gain:** 1 + (1k + 250kA pot) / 1k, so 2x to 252x (48 dB), 16 Hz leg corner, 47 pF across the feedback. This is far above a DS-1's 1 + 100k/4.7k = 22.3 (documented in `MODELED_GEAR.md`).
- **First stage:** fixed network (220k feedback, 100 pF; a transistor-assisted gain leg built around Q010).
- **Fixed filtering before the diodes:** high-pass 106 Hz (C033 0.015 / R043 100k), then a low-pass at about 680 Hz (R045 10k / R042 10k / C031 0.047), then a 48 Hz high-pass. **After** the diodes: a shelving cut (R032 10k, R031 4.7k + C023 0.015) of about -10 dB above 700 Hz to 2.3 kHz.
- **Second gain stage:** 4b with a 5.3k feedback (printed; could be 3.3k) and two more transistor-assisted legs (Q007, Q008); then 4a inverter -1.
- **Tone section after the clipping:** semi-parametric mid, 200 Hz to 5 kHz (panel print), 100k MIDDLE pot, dual 50kC MID FREQ pot, 0.022 uF / 2.2k / 0.0082 uF / 2.2k network; HIGH/LOW 100k pots with 22k, 10 pF feedback; a 1b follower with a 0.22 uF / 0.047 uF / 2.2k / 100k high-pass (about 105 Hz) bootstrapped on the LOW wiper; level 50kA; mid-level range in dB is **not printed**.
- **Output:** 10k / 1k, 1 Mohm in, 1 kohm out, -110 dBm noise, 20 mA.

Disagreement with the docs: **none directly**, since the docs have no MT-2. Two comparisons worth noting: (1) the docs' DS-1 stage gain (22.3) versus the MT-2's 252 at maximum; (2) `MODELED_GEAR.md` Tight mode puts 10k + 500k drive with 1N4148, while the MT-2 uses 1SS133 (a Japanese silicon switching diode; similar in behaviour to the 1N4148, but a different part number).

## D. Pedals checked in the docs but not given a schematic here (for completeness)

- **Boss DS-1:** `CIRCUITS.md` "Applied". The three new sheets do not contain a DS-1; the only overlap is the 2.2k clipper resistor in the MT-2 (above).
- **ProCo Rat:** `CIRCUITS.md` "Read only" (gain 1 + R / 43.4 ohm, tone 475 Hz to 32 kHz). The DOD 250 is the closest relative in the supplied set (op-amp, 1N4148 clipper to ground), but its gain law (1 + 1M / (4.7k + pot)) and its clipper (hard diodes after the op-amp, series 10k) differ from the Rat's, so no values transfer.
- **"Distortion" (generic) and "Metal" pedals:** nothing in the three docs has a component-value source beyond what is listed above.

## E. Suggested follow-ups (not done, no code changed)

1. Ask for the **DC30 Channel 1 preamp sheet** (to settle the 1.5k assumption) and the **supply sheet** (for the B+ voltages, which this sheet marks only as nodes 1, 2, 3).
2. Check the **47 nF vs 4.7 nF** gain-leg cap used for Distortion+ against the DOD 250's 4.7 nF.
3. Consider whether the **Cut shelf corner (2.8 kHz)** should differ between Chime Thirty (4.7 nF) and Dual Thirty (2.2 nF).
4. If an MT-2 style is wanted, the values in `MetalZone_MT2.md` are complete except the items marked (?) there (diode orientation, which transistor lead is the emitter, 1b output destination, R030).
