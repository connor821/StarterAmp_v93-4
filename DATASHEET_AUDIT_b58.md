# Datasheet audit (b58): how far off is the model from the sheets?

Report only. No model was changed. Nothing here was listened to; the numbers are model values against what is printed on the sheets
filed in `reference/datasheets/`. The sheets give limits and typical figures, not a measured part, so "off" below means "outside or at
the edge of what the sheet allows", not "wrong by ear".

## Summary

| Area | Verdict | Biggest gap |
|---|---|---|
| Diodes (silicon, germanium) | Close | Silicon is 0.69 V at 10 mA, under the sheets' 1 V maximum (typical is about 0.7 to 0.8 V, so 0.05 to 0.1 V low at heavy current). Germanium has no usable sheet. |
| Transistors | Close where there is a model | Only the treble-boost stage is a transistor circuit; its hFE of 100 sits on the OC44's minimum (100), so real parts are the same or higher. |
| Op-amps | Mixed | Stage bandwidths assume 1 MHz (RC4558 class, sheet typical 4 MHz) and 3 MHz (TL07x class, sheet typical 5.25 MHz). Conservative by 1.7x to 4x, which only matters at the top of the gain range. |
| Tube rectifier | Plausible | Source resistances 120 to 250 ohm sit inside or above the sheets' 21 to 97 ohm tube plate resistance plus transformer winding. No amp has a figure read from a rectifier sheet. |
| Speakers | Mixed | Hand-built speakers: resonance 68 to 110 Hz against sheet Fs 75 to 85 Hz (up to +30 %). Cab-fitted "Studio / Classic / Compact" speakers carry 200 to 320 Hz bumps from recordings; those are not comparable with a driver Fs. |

## Diodes

| Model | Parameters | Voltage at 0.1 / 1 / 5 / 10 mA | Sheet |
|---|---|---|---|
| Silicon | Is 2.52e-9, nVt 0.0453 | 0.48 / 0.58 / 0.66 / 0.69 V | 1N914 and 1N4148: Vf max 1 V at 10 mA; reverse current 5 uA at 75 V. Model is inside the limit, about 0.1 V under a typical part at 10 mA. |
| Germanium | Is 2.6e-7, nVt 0.034 | 0.20 / 0.28 / 0.34 / 0.36 V | The 1N34A file gave no extractable text. 0.28 V at 1 mA is the usual figure for a small germanium diode; I could not confirm it from the sheet. |
| LED | Is 1.4e-13, nVt 0.075 | 1.53 / 1.70 / 1.82 / 1.87 V | No sheet. |

**Wrong file:** `1N60.PDF` is a power MOSFET datasheet, not the 1N60 germanium diode. Nothing was taken from it. If you have the real
1N60 sheet, send it and it replaces this one.

## Transistors

| Part | Sheet | In the model | Gap |
|---|---|---|---|
| OC44 (germanium PNP) | hFE min 100, ft 8 MHz, Cc 12 pF | Treble-boost stage: hFE 100, gain 78.7 (published 80) | On the sheet minimum. A typical part has more gain; the model is the low end. ft and Cc are not modelled (the stage's own roll-off is not in the analysis the circuit came from). |
| 2N5088 | hFE 300 to 900 at 100 uA, hfe 350 to 1400, ft 50 MHz min | No circuit uses it | None to compare. The sheet's gain is 3x to 9x the OC44 minimum, so a silicon boost would run much hotter. |
| 2N3903 | filed | No circuit uses it | None. |
| BC107 / BC108 | BC108 hFE 110 to 800, ft 150 MHz | The Wall fuzz booster is a common-emitter stage with 16.7 dB gain and a 1.2 kHz pole | The pole comes from the circuit, not from the transistor. The sheet's ft is far above it, so the transistor is not the limit. |
| AC107 | table scan, no extractable text | None | Not compared. |

## Op-amps

| Chip | Sheet | Model assumes | Ratio |
|---|---|---|---|
| RC4558 | GBW 4 MHz typ; slew 1.1 to 2.2 V/us | 1 MHz (Op-Amp Distortion kind, Stack Drive, Wall fuzz option) | model 4x lower |
| TL072 / TL082 | The filed sheet is the newer TL07xH (GBW 5.25 MHz typ; slew 20 V/us); the classic TL07x / TL08x parts are 3 MHz typ | 3 MHz (overdrive Open, Klon-style default) | model matches the classic part; not changed |
| LM308 / 108 | GBW depends on the compensation capacitor; text not extractable | 1 MHz (Distortion pedal, 741 class) | not checked |
| 741 class | not filed | 1 MHz | standard figure |

What it means: GBW sets a pole at GBW divided by the stage gain. At gain 100 the 1 MHz assumption puts the pole at 10 kHz and the 4 MHz sheet
value at 40 kHz. So the model is slightly darker than the real chip at the top of the gain knob, and unchanged elsewhere. Slew rate is not
modelled anywhere.

## Tube rectifier

The sheets (RCA 5U4-GB and Amperex 5AR4) give effective plate resistance of about 21 to 97 ohm with a 40 uF input capacitor, 1500 to 1550 V peak
inverse.

| Case | Amp type | Model source resistance | Comment |
|---|---|---|---|
| 9 | tweed combo, 5Y3 | 250 ohm | A 5Y3 is a smaller tube than either sheet; no sheet to compare. |
| 28 | Dual Thirty, rectifier tube | 140 ohm | Read from its sheet: 350 V at the reservoir. Inside the range (tube 21 to 97 plus transformer). |
| 33 | Brit Eighteen, rectifier tube | 120 ohm | Estimated. Plausible. |
| 4 | 100 W dual-rectifier amp | 80 ohm | Described as between silicon and tube. Plausible for a 5AR4-class pair. |
| Silicon cases | American, Hi / Lo, others | 60 to 85 ohm | These are mostly the transformer; the rectifier adds nothing. |

The silicon menu option multiplies the sag by 0.35. Against the sheets that is right in direction (no tube drop) but the factor is a
choice, not a measurement.

## Speakers

Sheet figures: Celestion Blue Fs 75 Hz, 100 dB; G12H Anniversary Fs 85 Hz, 100 dB; G12M Greenback Fs 75 Hz, 98 dB; Vintage 30 Fs 75 Hz, 100 dB
(range 70 to 5000 Hz). The Eminence sheets add the Wizard, Tonker, Swamp Thang, Ragin Cajun, Wheel House and others.

| Group | Low-cut corner | Resonance | High-cut | Against the sheets |
|---|---|---|---|---|
| 13 hand-built speakers (Modern Tight to Mid Forward) | 50 to 90 Hz | 68 to 110 Hz | 4.3 to 8.5 kHz | Resonance is 10 to 30 % above Fs for the "vintage" voicings (Chimey Alnico 110, Rock Bite 110, Mid Forward 105 vs Fs 75 to 85). Modern Tight 78, Hi-Fi Full 70 and Modern Scoop 74 sit right on it. High-cut medians about 5.5 kHz against the sheets' 5 kHz upper range. |
| Studio, Classic, Compact, Raw sets (fitted to recordings) | 30 to 154 Hz | 76 to 320 Hz | 3.5 to 6.3 kHz | Not comparable. A mix of mic, cab and room is fitted into the resonance slot, so values of 200 to 300 Hz are cabinet and room effects, not driver Fs. |
| Ideal Reference / Warm / Presence / Wide Open | 60 to 80 Hz | 80 to 105 Hz | 4.8 to 9 kHz | Reference and Warm sit close to a Celestion curve. Wide Open goes past the sheets' 5 kHz on purpose. |

The sheets' frequency-response graphs were not digitised, so shape between 1 and 5 kHz (the "presence rise" and the breakup bumps) is not
checked, only the corner frequencies and Fs.

## Follow-up (done in b58, in the order asked)

1. **Vintage speaker resonances:** Vintage Crunch 100 to 88 Hz, Rock Bite 110 to 90, Chimey Alnico 110 to 85, Mid Forward 105 to 90. The Eminence sheets list Fs from 70 Hz (DV-77) to 109 Hz (Tonkerlite), with the Wizard 89, Ragin Cajun 87, Swamp Thang and Tonker 97, Wheel House 82, so the old 100 to 110 values were not unreasonable for an Eminence-type driver; the new values follow the Celestion sheets (75 to 85 Hz).
2. **Op-amp bandwidth:** only the two stages that name a 4558 changed (the Wall fuzz's op-amp build and the Tight overdrive's circuit mode), 1 MHz to 4 MHz. Both have feedback capacitors that set a lower pole (2.3 kHz and a gain of 5.7), so the change is inaudible by design. The TL07x stages stay at 3 MHz because the filed sheet is the "H" version; the classic part is 3 MHz.
3. **Curves digitised** from all four Celestion sheets (raster plots, read by colour) and eight distinct Eminence plots (vector plots, read exactly): Blue, G12H, Greenback, Vintage 30, GA-SC-X (also GA10-SC64), Ragin Cajun, Swamp Thang, Tonker, Wizard (8 and 16 ohm), Tonkerlite, Wheel House 200. Not read: DV-77 and GA10-SC69 (raster, no plot found in the file) and Wheel House 150 (grid not recognised). Data: `reference/datasheets/speakers/DRIVER_CURVES.csv` (1/6-octave, 100 Hz to 5 kHz).

**Comparison (level matched, 100 Hz to 5 kHz, RMS dB between the model speaker alone and each driver):**

| Group | Typical gap | Reading |
|---|---|---|
| Hand-built vintage and modern speakers (Chimey Alnico, Mid Forward, Tight Ceramic, Rock Bite, Modern Tight, Ideal series, Hi-Fi Full and so on) | 1.4 to 3.2 dB | Close to the real drivers; the closest are Chimey Alnico (1.4 to 2.6 against every driver except the Wheel House 200), Mid Forward and Tight Ceramic. |
| Smooth Lead, Vintage Crunch, Compact series, Heavy, Modern Scoop | 3 to 4 dB | Fair. |
| Studio, Classic, Rhythm, Raw, American, PA sets (fitted to mixed recordings) | 4 to 11 dB | Not meant to match one driver; Classic Dark and Raw Big Low are furthest (10 to 11 dB) and are voiced, not measured. |

**The one systematic miss** (average of the 12 driver curves against the average of the 13 hand-built speakers, each level matched):

| Hz | 100 | 200 | 320 | 800 | 1000 | 1600 | 2000 | 2500 | 3200 | 4000 |
|---|---|---|---|---|---|---|---|---|---|---|
| Drivers, dB | -3.9 | -1.8 | -2.6 | +0.1 | +0.8 | -1.4 | +5.1 | +6.8 | +5.5 | +2.0 |
| Model, dB | 0.0 | -0.5 | -0.6 | -1.2 | -0.6 | +1.2 | +1.7 | +2.2 | +1.9 | +0.7 |
| Model minus drivers | +3.9 | +1.3 | +2.0 | -1.3 | -1.4 | +2.6 | -3.4 | -4.6 | -3.6 | -1.3 |

So real 12-inch guitar drivers have a presence rise of 5 to 7 dB at 2 to 3 kHz after a dip near 1.6 kHz; the model's average speaker has about half of it (+2 dB). The model also has about 2 to 4 dB too much at 100 to 300 Hz. The mic and cabinet blocks add more on top, so the full chain may already make up part of it; I have not measured that. The spread between real drivers is 2 dB at 1 to 2 kHz and about 5 dB above 4 kHz, so the 2 to 3 kHz rise is the one thing all of them share.

## Not changed, offered

- (Superseded by the IR check below: the finished IRs do not show the sheets' 2 to 3 kHz rise, so it was not added; the 1.6 kHz dip and the 150 Hz hump were added instead.) Give the hand-built speakers a shared 2 to 3 kHz presence rise (about +3 dB more) and a 1.6 kHz dip, and lower their low end by 2 to 3 dB. This moves every cab preset that uses them, so the full-chain response should be checked first.
- Replace the wrong 1N60 sheet.

## Check against your IR folders (b58, analysis only, nothing changed)

Source: Ownhammer MAR-66 2x12 closed cab (one dynamic-mic IR each for the Alnico Blue, G12M, G12H and several V30 builds; same cab and mic, only the speaker changes) and ML Sound Lab's Freeman pack (one V30 cab and one M25 cab, each recorded with a 57 at three spots and with a 58, 421, 121, 160, 184, 7B and 906). Curves are 1/6-octave magnitude averages of the IRs, level matched. One cab and one mic setup only, so read these as a first look, not a verdict.

**1. Do the sheets and the IRs agree on how the speakers differ?** Roughly. With the V30 as the reference, the IR difference between two speakers and the sheet difference between the same two agree to 1.3 to 1.5 dB RMS (correlation 0.35 to 0.58). The speakers differ from each other by only 1.0 to 1.5 dB RMS in either source, so the real differences between these four drivers are small compared with what the cab and mic do.

**2. Sheet against IR (same speaker).** Level matched, 4.5 to 5.1 dB RMS. The IRs do not show the sheets' 2 to 3 kHz rise (+6 to +7 dB on the sheets, 0 to +2 dB in the IRs), they show a deep dip at 1.1 to 1.6 kHz (-4 to -6 dB), and they have 4 to 6 dB more at 140 to 200 Hz (the cab). So the sheet curve is the driver alone and the cab, mic and spot reshape it a lot. This partly answers the earlier finding: the model's speaker-only curve is missing the sheet's presence rise, but the finished IRs do not have that rise either.

**3. Model chain against the IRs** (every model speaker with the Flat mic, 2x12 Closed, position 0.3, 1 in; RMS dB to each IR, 100 Hz to 5 kHz):

| Group | RMS gap |
|---|---|
| Closest speakers: Studio Standard, Studio Smooth Tight, Heavy Tight, Studio Hollow, Heavy Bright | 1.4 to 2.6 dB (the fitted-from-recordings group) |
| Median of the 49 speakers | 3.2 to 3.4 dB |
| Average of the 13 hand-built speakers against the average of the four real IRs | high by 3.8 to 5.6 dB at 1.3 to 2 kHz, low by 2.6 to 4.5 dB at 125 to 250 Hz, within 1 dB above 2.5 kHz |

So the hand-built speakers miss in two places when they are put through the cab: a missing 1.2 to 2 kHz dip and too little 125 to 250 Hz (the cab's box region). The recording-fitted Studio speakers already carry both.

**4. Mics.** The Freeman mics differ from the 57 by at most 2.3 dB RMS between 100 Hz and 5 kHz, so the model's mics (checked in the same way) are fine in that range: the best model mic is within 0.4 to 1.5 dB of each real one. Where the real mics differ is outside that range:

| Mic vs 57 | 50-100 Hz | 3-6 kHz | 6-10 kHz | 10-16 kHz |
|---|---|---|---|---|
| 421 | +3.9 | +3.8 | +2.5 | +6.5 |
| 121 (ribbon) | +8.9 | -2.5 | -3.4 | -7.7 |
| 160 | +3.1 | +0.1 | +1.2 | -1.9 |
| 184 (small condenser) | +4.2 | -2.4 | -1.3 | +4.4 |
| 7B | +2.3 | +1.0 | +2.5 | +4.1 |
| 57 position E (edge) | -0.1 | +2.0 | +4.7 | +4.2 |

Mic datasheets would help most there, because the model's speakers roll off at 4 to 6 kHz and the mic models are the only thing shaping 6 to 16 kHz and the proximity region below 100 Hz. Useful sheets: SM57, SM7B, MD421, R121, M160, e906, 414 / small condensers (frequency response plot to 15 or 20 kHz, proximity effect curves if printed).


## Which layer owns what (b58, applied to the 13 hand-built speakers)

Measured on 74 Ownhammer speakers recorded in one 2x12 with one dynamic mic, and on ten ML Sound Lab cabs recorded with a 57 at the same spot (300 to 1000 Hz matched). Differences between speakers are small (1.6 dB RMS between 100 Hz and 5 kHz) and so are differences between cabs (1.6 dB RMS); nearly all of the shape is shared:

| Feature (real IRs) | Size | Owner | Why |
|---|---|---|---|
| Fs resonance, upper-mid and top-end character, cone breakup | speaker to speaker, 1 to 2.5 dB up to 3 kHz, 3.5 to 5 dB above 4 kHz | **Speaker** (kept as it was: each of the 13 keeps its own values) | This is the only part that differs between speakers. |
| Hump at 150 Hz (+5 dB) and drop below 90 Hz (-7 dB) | same in the 2x12 and in all ten (mostly 4x12) cabs | **Cab (box)**: full on closed and ported cabs, 40 % on open backs, 60 % on the 1x8 | Close-mic proximity and the closed box. Open-back size could not be checked (no open-back IRs in the folders), so 40 % is a guess. |
| Dip at 1.6 kHz (-5.4 dB) | same for all speakers, all cabs, and all three 57 positions | **Mic (the reference dynamic mic on the cone)**: shared by every speaker, not stored in any one | The sheets show only part of it (-1 to -3 dB), the rest is the mic on the cone. |
| 2.3 dB shelf above 3.2 kHz and a 2nd-order roll-off at 5.1 kHz | similar everywhere | **Mic** | Same reason. |
| Mic choice and Mic position | below 100 Hz and above 6 kHz, plus up to 4 dB at 3 to 6 kHz | **Mic and position** (unchanged; the mic datasheets are the next input) | They do not move the 100 Hz to 3 kHz shape (at most 2.3 dB). |

How it is built: each speaker has a flag, `refBase`, true for the 13 hand-built speakers and false for the ones fitted to recordings (Studio, Heavy, Compact, Classic and so on, which already contain the shared shape, and the Ideal and Big Fifteen speakers, which are meant to be neutral). The shared shape is added only when the flag is set. Nothing else changed: speaker values, cab terms, mic terms, position and distance are as they were.

**Result** (RMS dB against the real IRs, 100 Hz to 5 kHz, hand-built speakers through 2x12 Closed or 4x12 Closed with the Flat mic and position 0.3):

| Check | Before | After |
|---|---|---|
| 13 speakers vs the average 2x12 / dynamic-mic IR (this data set was used for the fit) | 3.7 | 1.75 |
| 13 speakers vs the average of the ten ML cabs, 4x12 Closed (not used for the fit) | 3.75 | 1.9 |
| Spread between the 13 speakers | 1.5 | 1.5 |

The variety between speakers is untouched; what changed is the shared shape under all of them. The residual error is mostly the cab-to-cab and speaker-to-speaker spread of the real IRs (1.6 dB each), so there is little left to fit from this data. Not heard.

## Mic sources received (notes only, no datasheets yet)

Two text files from you are filed in `reference/datasheets/mics/` (`MIC_SOURCES_notes_1.txt`, `_2.txt`): a list of manufacturer pages and PDFs (SM57, SM7B, MD 421-II, R-121, M 160, M 201, e906, C414 XLS, U 87 Ai, Audix i5, RE20 and others). Links in them are unverified, and the MD 421-II one is marked as not found. Nothing was fetched or used yet; the datasheets you are pulling will be.

How I plan to use them, given the IR findings: the speaker models already contain a reference dynamic mic (the 57-type response, which every 57 IR in your folders agrees on), so a mic sheet is applied as the difference from the SM57 curve, not as an absolute curve. Otherwise the 57's shape would be counted twice. The IR-measured differences above (421, 121, 160, 184, 7B, and 57 at the edge) are the cross-check for each sheet curve, and the M 160 and M 201 distance curves (10 cm and 1 m; 2, 10 and 1 m) are the main source for the proximity term, which today is a single low shelf.


## Mic datasheets: what the sheets say (b58, report only, no mic changes made)

Curves were read from the plots (digitised for SM57, e906, MD 421-II; read by eye from the plot for the rest, so roughly +/-1 dB) and written to `reference/datasheets/mics/MIC_CURVES.csv` (dB re 1 kHz, 31 points 40 Hz to 16 kHz). Table: average dB difference from the SM57 sheet curve per band, next to the difference measured on the real IRs (your folders).

| Mic | 50-100 | 100-300 | 3-6k | 6-10k | 10-16k | Real IR (same bands) |
|---|---|---|---|---|---|---|
| MD 421-II (M position) | +5.4 | +1.6 | +3.1 | +1.9 | +4.7 | +3.9, -0.5, +3.8, +2.5, +6.5 |
| R-121 | +8.1 | +1.5 | -3.2 | -3.7 | -0.6 | +8.9, +0.9, -2.5, -3.4, -7.7 |
| M 160, 1 m | +0.4 | +2.0 | -2.0 | -2.5 | -1.5 | +3.1, +0.3, +0.1, +1.2, -1.9 |
| SM7B (flat) | +4.5 | +0.6 | -3.7 | -3.0 | -2.5 | +2.3, -0.3, +1.0, +2.5, +4.1 |
| e906 (flat state) | -1.9 | -0.8 | -1.2 | -4.4 | -5.4 | +1.0, -1.9, +1.0, +0.8, -1.5 |
| M 201, 1 m | +0.8 | +1.5 | -0.5 | -0.2 | +3.7 | no IR |
| R-10 | +7.9 | +1.1 | -3.8 | -5.6 | -2.4 | no IR |
| Audix i5 | +8.3 | +6.1 | +2.6 | +1.1 | -0.7 | no IR |
| e609 (1 m) | +2.7 | +1.2 | +2.3 | -1.0 | -0.2 | no IR |
| RE20 (switch off) | +6.3 | +1.6 | -4.0 | -2.7 | +2.0 | no IR |
| U87 Ai (cardioid) | +7.2 | +1.6 | -4.0 | -2.5 | -0.6 | no IR |

Findings:
- **MD 421-II and R-121 agree with the IRs** (421 within 2 dB in every band; R-121 within 1 dB up to 10 kHz). The 121 sheet is too smooth above 10 kHz: the IR shows -7.7 dB where the sheet plot shows about -1, so for ribbons the IR beats the sheet.
- **SM7B, e906 and M 160 disagree with the IRs at 3 kHz and up** by 3 to 6 dB (sheets say darker than the 57, IRs say similar or brighter). The most likely cause is the sheet's SM57 curve, which has a +5 to +6 dB presence peak at 5 to 6 kHz that is a free-field, on-axis figure; on a cab the 57 lies lower and the other mics are not darker than it. So the sheet differences above 3 kHz are not trustworthy for the models; below 300 Hz they are.
- **Proximity**: the plots give clean numbers for the single low shelf. M 160: 1 m to 10 cm adds +13 dB at 50-100 Hz, +9 dB at 100-300 Hz. M 201: 10 cm adds +9 dB at 50-100 Hz, 2 cm adds +17 dB. e609: 5 cm adds +14 dB at 50-100 Hz and +11 dB at 100-300 Hz. The boost peaks at 100 to 130 Hz in all three and has fully gone by 1 kHz. The model's shelf (180 Hz corner, 3.5 or 10 dB scaled by distance) is the right shape; the sheets suggest a 120 Hz centre and about 12 dB at 5 cm, with a steeper fall above 300 Hz than a shelf has. RE20 (Variable-D) has none, which the Ribbon/Dynamic proximity values of 0.5 to 1.0 cannot represent.
- Model mics against the sheets: not yet compared numerically (the model mics were fitted to the IRs, and the IR column above is the cross-check that counts). In outline, the ribbons (bass lift, roll-off at 5 to 7 kHz) point the same way as the R-121 and R-10 sheets. Condensers: no sheet curves for the C414 and AT4050 (specs and polar patterns only; roll-off switches 40/80 Hz 12 dB/oct, 160 Hz 6 dB/oct). The U87 sheet shows +2 dB at 8 to 10 kHz and about -9 dB at 20 kHz.
- **Warm Audio and the other earlier sheets, rechecked** (my first pass wrongly said "no plots"):
  - WA-8000 (p10, cardioid and omni), WA-67 (p7, cardioid, omni, figure-8) and WA-47 (p8, three patterns) have response plots. WA-8000 cardioid is flat within about 1 dB to 5 kHz with a +4 dB peak at 10 kHz and -3 dB at 20 kHz. The WA-67 cardioid is flat from 50 Hz (-10 dB at 20 Hz), +1 dB at 8 to 10 kHz, -7.5 dB at 20 kHz, and its figure-8 has a 3 dB shelf below 300 Hz. The WA-47 cardioid is flat within about +/-2 dB to 20 kHz (the omni and figure-8 plots show ripples of up to 3 dB).
  - Their bands against the SM57 sheet curve (50-100, 100-300, 3-6k, 6-10k, 10-16k): WA-8000 cardioid +7.8, +1.9, -2.5, -1.6, +3.1; WA-67 cardioid +7.2, +1.6, -3.6, -3.1, -0.5; WA-47 cardioid +7.2, +1.6, -4.0, -4.0, -0.7. All three are in `MIC_CURVES.csv`.
  - AT4050 cardioid (page 2, 10 dB scale): flat, +1.5 dB at 6 kHz, +4 dB peak at 10 kHz, -9 dB at 20 kHz; the roll-off switch curve is -11 dB at 20 Hz, -7 at 50 Hz, -3 at 100 Hz. C414 XLS (polar pattern PDF, five plots): flat to within +/-2 dB, a -1.5 dB dip at 2 kHz and about +2 dB at 10 to 12 kHz in the cardioid, with the 40 Hz and 80 Hz 12 dB/octave and 160 Hz 6 dB/octave filter curves drawn.
  - No plots in WA-251, WA19, WA44, CX12, CX24 and Retro 64 (spec text only; the WA44 ribbon lists 20 Hz to 20 kHz within +/-12 dB, which is a rated range, not a curve).
  - Note on every flat condenser above: the "+7 dB at 50-100 Hz" against the SM57 sheet is mostly the 57 curve being low there (it is about -10 dB at 50 Hz on its sheet), not bass on the condensers.

## Other sources filed this session (not used yet)

- `reference/schematics/1176/`: UREI 1176LN schematic (suffix K) and condensed notes from the Mason Audio build guide (Rev. F: one BF245A GR FET, feedback sidechain, measured ratios 5.1/8.2/11.5/20.5 for 4/8/12/20).
- `reference/compressors/SOS_Fairchild_660_670_notes.md`: time constants, ratio behaviour and topology from the Sound On Sound review. No schematic yet.


## Mic drop 3 (b58, report only)

Ten more files in `reference/datasheets/mics/` (the e609 sheet was a duplicate). Curves read by eye (+/-1 dB) and added to `MIC_CURVES.csv`: M 88 (2 cm, 10 cm, 1 m), D112 MkII (1 m and 10 cm), MD 441-U (music position) and KSM32 (15 and 60 cm). Band differences from the SM57 sheet curve (50-100, 100-300, 3-6k, 6-10k, 10-16k):

| Mic | 50-100 | 100-300 | 3-6k | 6-10k | 10-16k |
|---|---|---|---|---|---|
| M 88, 1 m | +1.0 | -0.2 | +0.1 | +0.1 | +1.8 |
| M 88, 10 cm | +10.6 | +4.5 | +0.1 | +0.1 | +1.8 |
| M 88, 2 cm | +19.7 | +12.3 | +0.1 | +0.1 | +1.8 |
| D112 MkII, 1 m | +9.9 | +4.2 | +0.3 | -3.1 | +1.0 |
| D112 MkII, 10 cm | +14.7 | +7.8 | +0.3 | -3.1 | +1.0 |
| MD 441-U (flat) | +7.2 | +1.6 | -3.9 | -3.5 | -2.7 |

- Proximity now has five clean data sets (M 160, M 201, M 88, D112, e609): moving from 1 m to 10 cm adds about +9 to +13 dB at 50-100 Hz and +4 to +9 dB at 100-300 Hz, peaking at 100 to 130 Hz; the 2 cm curves add 7 to 10 dB more. The KSM32 sheet (Figure 4, page 7 of the guide, which I first misread as having one distance) agrees in shape: 15 cm vs 60 cm is +6.8 dB at 50 Hz, +4.5 at 100 Hz, +2 at 200 Hz, gone by about 400 Hz. Its 60 cm curve is flat from 25 Hz to 3 kHz, then +1 at 3.5 kHz, +2.6 at 6.5 kHz, a dip of -1 at 9 kHz, +1.8 at 10.5 kHz and -4 at 20 kHz. Against the SM57 sheet curve (50-100, 100-300, 3-6k, 6-10k, 10-16k): 60 cm +7.2, +1.6, -3.2, -3.3, +0.4; 15 cm +13.3, +4.1, -3.2, -3.3, +0.4. Both are in `MIC_CURVES.csv`.
- **SM58** (page 6 plot, missed on my first pass): 0 dB at 100 to 1 kHz, -7.8 dB at 50 Hz, +1.3 at 2 kHz, +4.9 at 5 kHz, a notch at 8 kHz, +4.2 at 10 kHz, -6.5 at 17 kHz. Against the SM57 curve: +3.0, +1.9, +0.1, -0.8, +1.1 (50-100, 100-300, 3-6k, 6-10k, 10-16k), so it is a 57 with a little more low end. Text: proximity +6 to +10 dB below 100 Hz at 6 mm.
- **AEA R84** (pages 9 and 10, also missed at first): front response -0.4 dB at 100-200 Hz, then a slow fall of about -1.3 at 1 kHz, -2.7 at 2 kHz, -3.8 at 5 kHz, -5.3 at 8 kHz, about -5 to -6 above 10 kHz, with a ripple of about 1 dB; the rear is similar (about -5.4 at 9 kHz, -8 at 20 kHz). Its proximity chart (50 Hz to 5 kHz): "very close" +3.5 at 50 Hz, +9 at 120 Hz, +5 at 300 Hz, gone by 700 Hz; "closer" +4.5 at 100-125 Hz; free-field -7.8 at 50 Hz. Against the SM57 sheet: front +7.8, +2.4, -6.2, -7.6, -4.0; free-field +1.0, -0.8, -3.0, -3.7, +0.3. The front plot and the proximity plot's free-field curve do not agree at 50 Hz (about 0 vs -7.8), so one of them is at a different distance or is smoothed.
- **Telefunken M80 / M81** (page 7, 1 kHz = 0): M80 -13.5 dB at 50 Hz, -5.3 at 100 Hz, +2.4 at 300 Hz, +7.5 at 4-5 kHz, +4.5 at 10 kHz, -4.3 at 20 kHz. M81 -11.5 at 50 Hz, -2.3 at 100 Hz, +5.8 at 300 Hz, +5.2 at 4-5 kHz, +2.2 at 10 kHz, -4.2 at 20 kHz. Against the SM57 sheet: M80 -1.4, +0.8, +3.0, +2.2, +1.5; M81 +0.6, +3.9, +0.7, -0.5, +0.1. Both are in `MIC_CURVES.csv`.
- MD 441-U: nearly flat from 40 Hz to 8 kHz (music position), with a +1.5 dB peak near 6 kHz and a roll-off above 10 kHz; a five-position bass roll-off (music, 1, 2, 3, speech; roughly -8 to -25 dB at 100 Hz, read by eye) and a +3 to +4 dB treble boost across 2 to 8 kHz. Against the SM57 sheet curve that is -3.9 dB at 3-6 kHz, which, like the SM7B and e906, is probably the 57's free-field presence peak and not a real darkness on a cab.
- C451 B: flat with a 75 or 150 Hz 12 dB/octave switch and 10/20 dB pads, plot too small to read. C1000S: specs only (80 Hz bass cut, 10 dB pad, presence boost adapter, no plot). Telefunken M82: a 350 Hz -6 dB kick notch and a +6 dB high boost from 2 kHz (its small chart is not read).


## b59 follow-up: what was changed because of the audit

**Mic distance (proximity).** Rebuilt as a physical law (see `MODELED_GEAR.md`, b59). Default-distance output is unchanged; only the change with distance differs. The rms error against sheet pairs of distances is 0.5 to 2.9 dB for 5 to 60 cm. The 2 cm curves (M 88, M 201) run 6 to 8 dB hot before the soft limit, so the lowest Distance settings are still the least trusted.

**Compressor styles against the filed sources.**

| Style | Source says | We had | Now |
|---|---|---|---|
| FET (1176-type) | measured ratios 5.1 / 8.2 / 11.5 / 20.5 for the 4 / 8 / 12 / 20 buttons; attack 20 to 800 us, release 50 ms to 1.1 s | the menu ratios as labelled (4 = 4) | 2, 3, 4, 6, 10, 20 read as 2.4, 3.7, 5.1, 6.6, 9.9, 20.5; attack and release already inside the published ranges |
| Variable-mu (Fairchild-type) | attack 0.2 ms (0.4 ms on three positions); release 0.3 / 0.8 / 2 / 5 s, auto 2 s + 10 s and 0.3 s + 10 s + 25 s; ratio about 1:2 to 1:30 | attack floor 3 ms (default 15 ms), release stretch 1 to 2.5 x | attack floor 0.2 ms (default 2 ms); release stretch 1 to 5 x with the squeeze memory |
| Bus (SSL-type) | attack 0.1 / 0.3 / 1 / 3 / 10 / 30 ms; ratio 2 / 4 / 10; release 0.1 / 0.3 / 0.6 / 1.2 s and auto (two time constants up to about 2.4 s) | attack floor 10 ms; ratio capped at 4:1; release 0.6 to 1.5 x | attack floor 0.1 ms; all ratios; release 0.5 to 3 x |
| Opto (LA-2A-type) | attack essentially instant (manual) or 10 ms (vendor); release about 60 ms for the first half then 0.5 to 5 s | two-stage release as published, 10 ms centre attack | unchanged |
| Distressor | attack 50 us to 30 ms, release 50 ms to 3.5 s, detector high-pass and band emphasis, 2:1 with a +15 dB soft knee | covered in part by Standard (detector HP, saturation modes) | not changed; band emphasis and the soft 2:1 are not modelled |
| dbx 160 | see `reference/compressors/dbx160A_spec_notes.md` | not present | new style RMS (VCA) |

Auto Level makeup rows for FET, Variable-mu and Bus were re-measured on the same plucked test signal after the changes; RMS (VCA) has its own row.

**Mic modes.** Every datasheet curve (1 m or equivalent, 50 Hz to 3 kHz, relative to the SM57 sheet) lies within about 2 dB rms of an existing mic. The closest matches: R-121, R-10, RE20, U87, MD441, KSM32, Warm Audio WA-8000/67/47, AT4050 and C414 are all near Ribbon Open (they share a flat midrange with +7 dB lift at 50-100 Hz that is an artefact of the SM57 sheet curve, not a real bass rise); SM58 near Dynamic Large; M88 and M80 near Condenser Detailed; MD421-II and e609 near Dynamic Hyper; M81 near Ribbon Dark. The differences that remain are above 3 kHz, where the SM57 sheet curve used as the reference is not reliable enough to model from. So no mic mode was added. What would help: close-mic cab impulse responses of the MD441, M88, e609, MD409 and R84 on the same cab and speaker as an SM57 (so differences can be read directly rather than through the sheets), and distance sweeps (1, 2, 4, 8, 12 in) of the same mic on one cab.


## Cab-set comparison (b61, report only; nothing in the sound was changed)

Method: peak-normalised IRs, so only shape is compared; every difference is aligned over a band where it should be zero, and the same mic/position settings are used for both sides. No IR content is stored anywhere.

**Cabinet type** (same V60 speakers, standard 4x12 vs the oversized ported 4x12, 57 / 121 / 421 at positions 00 to EDGE): the real difference is one bump of about +5 to +6 dB centred near 90 to 100 Hz (80 to 125 Hz wide). Below 70 Hz it is flat to +2 dB, and from 150 Hz up it is within about 1 dB. The model's Oversized Straight does the opposite: +5.7 dB at 50 Hz and +4.9 dB at 63 Hz, then -2 dB from 100 to 315 Hz. Against the absolute oversized IR the model is also about 13 to 15 dB too strong at 50 to 63 Hz, 3 to 5 dB too bright at 2.8 to 3.6 kHz and too dark above 8 kHz. Suggested fit (not applied): move the 62 Hz bump to about 95 Hz, drop the 300 Hz cut, raise the high-pass.

**Position** (57 and 421, positions 00 / 05 / 08 / 10 against 03): the model's position law matches within 1 to 2 dB (at 10: IR -4 to -5 dB above 3 kHz, model -5). Two real positions lie outside the model's range: EDGE (cone rim) is about -9 dB above 2 kHz, and FRED (centre, angled) is +3 to +6 dB at 3 to 6 kHz.

**Mics** against the SM57-type reference at position 03: ribbon (121) matches in the lows (+5 dB at 100 Hz) but is about 3 dB less dark than the model at 8 to 10 kHz; the large condenser (414) is 4 to 8 dB darker than the 57 at 3 to 5 kHz, where the model only dips 2 dB, and brighter than the model above 10 kHz; the 421 is 2 to 4 dB darker than the 57 above 4 kHz, where the model has a +3.5 dB bump at 4 kHz.

**Speakers** (V30 / EV / M20 / M75 / H75 in one cab): real differences are only 2 to 6 dB and mostly narrow; the model's speaker voicings are wider than that on purpose (they are styles), so nothing to correct.

Sets that allow more of this: the Ownhammer 412 MES-ST / TR / OS (same speakers, three cab types), 412 MAR-CB V30 / EVM / M20-PR / M75-PR / H75-PR (five speakers in one cab), the Pre-Rola M25 multi-mic collection, and 412 BOG-ST vs EDVH, ORNG vs ORNG M25 (cab or speaker swaps).


## Cab refit and No cabinet (b62; measured against the filed IR sets, not listened to)

What changed: the two oversized 4x12 cabinets (Straight / Angled) and the oversized 2x12 were refitted with the b61 findings: the strong low bump moved from about 62 Hz to about 95 Hz (the ported cab's real bump, +5 to +6 dB, 80 to 125 Hz wide), the 300 Hz cut was dropped, the low cut was raised so the model is no longer 13 to 15 dB too strong at 50 to 63 Hz, and the top was opened a little (the model was 3 to 5 dB too bright at 2.8 to 3.6 kHz and too dark above 8 kHz in the b61 comparison, now closer). The ML Sound Lab OS (oversized) and TR (traditional) sets were used as a second check: the OS-minus-TR difference follows the same single bump near 90 to 100 Hz, flat below 70 Hz to within +2 dB and within about 1 dB above 150 Hz.
Position range: now -0.4 to 1.6 (was 0 to 1), so the EDGE (cone rim, about -9 dB above 2 kHz) and beyond-the-rim positions are reachable; FRED-style centre-angled positions (+3 to +6 dB at 3 to 6 kHz) are still not separately modelled.
No cabinet: speaker only, no box resonance and no cabinet low cut; it is not fitted to any IR (no IR has no box).
Mics, speakers and the other cabinet types were left unchanged.
