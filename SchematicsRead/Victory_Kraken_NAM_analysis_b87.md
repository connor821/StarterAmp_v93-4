# Victory Kraken captures against our amps (b87, analysis only)

Nothing was built from this. 19 of the 33 supplied NAM captures were run through the fitting tool (`fitcapture --nam`, the 187-voicing library, no refine pass, default Hybrid mode, cab and pedals off). Voicings below are 1-based as the tool prints them (V1 = first voicing). **Measured, not listened to.** The score is the tool's weighted error in dB (lower is closer; about 2 is a good match, above 3 is a loose one). Every match is a candidate only; the tool compares level curve, 2nd and 3rd harmonic and octave bands, not feel.

| Capture | Gain slope (1.0 = no squash) | Best fit (score) | Next best |
|---|---|---|---|
| High-Gain 1 | 0.07 | Marque One V1 (2.08); free drive: Clarion V2 (1.71) | Marque One V3, Ember V1 |
| High-Gain 2 | 0.08 | Marque Dream V5 (2.08) | Clarion V2 (2.11), Revved Up V2 |
| High-Gain 3 | 0.03 | Marque Dream V5 (2.26) | Marque One V3 |
| High-Gain 4 | 0.08 | Marque Dream V1 (1.58) | Power Brawl V4 (1.60) |
| High-Gain 5 | 0.06 | Marque Dream V5 (1.47) | Bumble Lead V3 |
| High-Gain 6 | 0.02 | 5555 Block V4 (3.56); free drive: Marque Dream V5 (2.38) | Marque One V3 |
| Amp DI CH1 G3 | 0.44 | Solid Metal V4 (3.21) | Dual Thirty V1 |
| Amp DI CH1 G5 | 0.41 | Cedar Ten V2 (2.01) | Brit Eighteen V2 |
| Amp DI CH1 G8 | 0.30 | Clarion V3 (3.17) | Eclipse V2 |
| Amp DI CH2 G7 | 0.03 | Master Cranker V4 (1.77) | Revved Up V2 (1.81) |
| Amp DI CH2 G9 | 0.01 | Master Cranker V4 (2.56) | Master Cranker V1 |
| A2 Gain-I G5 | 0.48 | Marque Dream V3 (3.12) | Marque One V1 |
| A2 Gain-I G10 | 0.40 | Marque One V4 (3.09) | Marque One V3 |
| A2 Gain-I G10 + treble boost | 0.37 | Marque One V4 (2.84) | Modern Maul V4, Clarion V2 (3.03) |
| A2 Gain-II G5 | 0.29 | Solid Metal V1 (3.16) | Digital Dread V2 |
| A2 Gain-II G10 | 0.07 | Marque Dream V5 (2.73) | 5555 Block V4 |
| Super Kraken | 0.05 | Solid Metal V3 (2.85) | Digital Dread V4 |
| Super Kraken + Klon-style boost | 0.03 | Solid Metal V5 (4.01) | Solid Metal V1 |
| Super Kraken Med Gain | 0.22 | Brit Eighteen V3 (3.66) | Solid Metal V1, Ironside V1 |

What it says:
- The high-gain captures are saturated at every input level (slope 0.02 to 0.09). The best matches sit at the top of our Gain range (0.65 to 0.80), so the real gap is "more gain", not tone.
- No single amp wins. Marque Dream's fifth voicing wins most, Clarion V2 is close on two and is the best "free drive" fit on High-Gain 1, and the amp-only CH2 captures land on Master Cranker and Revved Up.
- The largest remaining errors are second harmonic (ours is higher: 3 to 10 dB off in the fits above) and octave-band shape.
- The clean-ish CH1 captures (G3, G5) are mid-slope and land on crunchy small amps, so there is no Kraken-like clean in the plug-in yet.
- Nothing here is a reason to retune an amp. Possible follow-ups, not started: a Clarion voicing with the pedal's "bass is rolled off before each clipper" behaviour (see `Victory_Kraken_pedal_clone_b87.md`), and a higher top of range on Clarion.
