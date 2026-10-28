# Amp build status and gaps (written against b62, report only: nothing in the plugin was changed)

Brand names appear here because this is a docs file; none of them are in the UI or source.

## 1. How the amps are built, and how the captures should be used

Every amp is built from some mix of these (the key used in `AMP_SOURCES.md`):

- **S** schematic values (stack, cathodes, supply, feedback), **C** captures, **M** manual or description only, **R** my own tuning from the amp's reputation, and **your ears** (nothing has been listened to yet; that is the final judge).

How I will use captures from here on, following what you said:

1. **Distortion character and feel come first.** The EQ section is powerful, so the spectrum a capture happens to have (knob settings, cab-less tone) is *not* something to chase. I stopped a fit I had started because it was spending its effort on the spectrum and bending the voicings to do it.
2. **A capture is a check, not a target.** Where a schematic exists, the schematic decides the structure and captures only adjust numbers the schematic cannot give (clipping softness, gain taper, sag). Where only reputation exists (R), captures are the first real evidence, and I would still show you before-and-after renders to listen to before anything is committed.
3. **No change goes in without a listen-first step** (a rendered comparison for you), because the score cannot judge feel.

### The character score used below

For each capture and each of our amp/voicing/Gain renders (744 renders of our 31 amps, no cab), the same 196 Hz sine at four levels (-36, -24, -12, -6 dBFS):

- **Compression shape**: how the gain falls as the level rises (rms difference in dB, from the lowest level).
- **2nd and 3rd harmonic at all four levels**, with anything below -60 dB treated as -60 (the NAM files have a numerical floor around -140 dB that our renders do not).
- Score = 0.6 x compression + harmonic error. **Spectrum is not in the score.** Around 2 or less is a very close character match, up to about 4 is good, 5 or more is a different distortion behaviour.

What the score still cannot see: sag and recovery (the "feel" of the power section), how the amp responds to pick attack, touch sensitivity, and how it behaves on chords. Those need either your ears or a dynamic test (bursts and decays against the NAM files); I can add that test if you want it.

### Captures I excluded or distrust

- **Matamp GT100 clean captures (2 of 3):** the level rises 45 to 50 dB for a 30 dB input change (an expander): the NAM model is broken at low level. Excluded.
- **Fender Tweed Deluxe 57:** output stops rising at -12 dBFS input (hard-limiting) and harmonics sit at the -140 dB floor below that. No amp of ours fits it (best 9.5); I would not fit to it.
- **Buttpussy (Orange clone), 3 captures:** no amp of ours gets below 10 (best 4.4 on one file). Too few and too odd to fit from.
- Boosted captures (pedal in front) are left out of unboosted fits; they are listed in `CAPTURE_COMPARISON_b62.md`.

## 2. What changed from the first report

The first report scored the spectrum as well. Judged on distortion character only, several "poor" verdicts were really tone, which your EQ already covers:

- **ENGL (Wreck Stack / Power Brawl):** Fireball Pack 1.7, Ironball 2.7, Savage 120 MK1 3.3, mk I 3.4, mk I higain 2.4, Powerball II 3.8. They are already close on character; only Fireball full gain (5.0) and Savage MKII (5.1) are not. So fitting the ENGL pair is more about extra voicings than about repairing the distortion.
- **Peavey 6505 1992 original** is 3.0 (best 1.0): the 5555 Block distortion is already close to the original 6505.
- **The Matamp captures belong to Matt's Amp** (you said so; my first table had put them against Hi-Wat Headroom).
- The remaining larger gaps are listed below.

## 3. Amp by amp

"Intended median (best)" = the median over the folder's captures of the best our intended amp can do (any voicing and Gain), and in brackets its single best capture; "any amp" = the same for the best of all our amps. A gap between the intended amp and "any amp" means other amps in our set behave more like those captures than the intended one does.

| # | Amp | Built from | Captures in this batch (count) | Character: intended median (best) vs any amp | Need |
|---|---|---|---|---|---|
| 0 | Twin Sparkle | S-part (Sparkly); R for Edge push, Boutique, Studio | none directly (Blues DeVille is a different amp) | n/a | low |
| 1 | Chime Thirty | S-part (Chime, Treble boost); R Normal, Jangle | AC30 (70) | 5.4 (2.1) and 4.1 (1.6) vs 3.3 and 2.0 | medium |
| 2 | Brown Eye | C (BE-100 and HBE captures) + schematic; R for Heavy, Brown, Mid push | Friedman BE-100 DLX (3), BE100 Deluxe (2) | 3.5 (2.0) vs 1.7; Deluxe 9.0 (7.0) vs 3.5 | low |
| 3 | Slow Burn | S-part (Smooth, Lead); R Crunch, Sustain; no captures | Soldano SLO100 (6), modded (25), LTD Teaser (2), SLO30 (2) | 7.3 (3.2) vs 4.7; 5.0 (2.7) vs 3.6; 5.3 vs 3.2; 6.5 vs 4.1 | **high** |
| 4 | Modern Maul | S-part + C (Mesa captures); R Chunk | Badlander Rectifier (56) | 4.0 (1.0) vs 2.9 | low |
| 5 | Acoustic | own design (R) | none | n/a | n/a |
| 6 | Club Crunch | S-part (Crunch, Jumped); R Pushed, Bright | shared with Plex Appeal | see 23 | medium |
| 7 | Bumble Lead | S-part + C (Dumble captures); R Blues | none in this batch | n/a | low |
| 8 | 5555 Block | S-part (Raw only); R Modern, Crunch, Buzz | 6505 1992 (51), 6505 (95), 5150 II (48), Blockletter (17), Stealth (17+3), Ivory (28), Invective (15) | 3.0 (1.0) vs 2.3; 4.1 (1.1) vs 3.0; 5.6 (1.8) vs 4.1; Invective 5.9 (3.2) vs 4.1 | medium (Modern to follow the Misha recordings, per your note) |
| 9 | Tweedle Dee | S (5E3 schematic); R Four-ten, Gritty | Tweed Deluxe 57 (7, unusable); Blues Deluxe Reissue (7) | 5.8 (3.8) vs 5.5 | low (no good data) |
| 10 | Jade Chorus | R, with JC-120 service notes (distortion stage applied, clean stack not) | none | n/a | low |
| 11 | Marque Dream | S-part + C; R Singing | none | n/a | low |
| 12 | Citrus Doom | all R | Sunn Model T (11) | 4.1 (1.7) vs 2.3 | medium (no schematic) |
| 13 | Revved Up | C (Revv); R Crunch, Polished | Synergy SYN-50 (48): no sibling | any amp 1.8 | low |
| 14 | Digital Dread | C (Thall and Bottom captures); R Industrial, Cavern | none | n/a | low |
| 15 | Master Cranker | S (2203 stack and first stage) + C (Jose captures); R Cranked, Boosted | JCM 800 2203 (30), Zakk Wylde (38), Jose Teaser (3) | 8.0 (4.4) vs 4.6; 6.1 (2.9) vs 3.6; 5.4 vs 5.4 | **high** (schematic-built Stock disagrees with real 2203 captures) |
| 16 | Harmonic Haze | S-part (Shiva schematic) + M; no captures | Bogner Shiva (17), Uberschall (16) | 2.7 (1.7) vs 2.0; 3.4 (2.1) vs 2.9; 3.1 (2.3) vs 2.2 | low (already close) |
| 17 | Diesel Fuel | C (47 VH4 captures) + service manual | none in this batch | n/a | low |
| 18 | Wreck Stack | S-part (supply, 6550 pair, presence, cathodes); R voicings; no captures | ENGL Savage 120 (MK1 8, MKII 3, mk I 6, higain 3), Fireball (18), Ironball (14) | 1.7 to 3.4 vs 1.1 to 2.4; Fireball full gain 5.0, MKII 5.1 | medium (add voicings) |
| 19 | Power Brawl | M + S (plate supply) + R | Powerball II (8) | 3.8 (2.4) vs 2.5 | medium |
| 20 | Stolen Fire | C (Prometheus captures); R Clean, Brilliant | none | n/a | low |
| 21 | Hi-Wat Headroom | S (tone stack, supply) | none (the Matamp captures go to Matt's Amp) | n/a | low |
| 22 | Console DI | M + R | none | n/a | low |
| 23 | Plex Appeal | S-part (Marshall stack; Jose-mod schematics) | Slash (51), Blackmore (114), Ceriatone (37), Super Lead (3 + 3), BJA (11), JTM 45 (50), Dookie (4) | 3.5 (1.6) vs 2.1; 4.5 (1.6) vs 2.5; 3.8 vs 2.6; BJA 8.6 (4.9) vs 3.7; JTM 45 4.8 (0.5) vs 3.8 | **high at low gain** (BJA, and JTM 45 outside its best settings) |
| 24 | Matt's Amp | S (three tone networks); R gain, clipping, sag; no captures | Matamp GT1 (6), GT100 (1 usable) | 5.4 (1.5) vs 3.4 | medium |
| 25 | Marmalade | S + C (Rockerverb captures); R Sharp | none | n/a | low |
| 26 | Eclipse | M + C; left alone | | | none |
| 27 | Marque Five | S-part + C | Rivera M100 OD (35): no sibling | any amp 2.0 | low |
| 28 | Dual Thirty | S-part (gain structure, supply, bright cap); R tone; no captures; no cathode value | Matchless DC30 clone (50) | **7.3 (3.4) vs 3.6** | **highest**: large gap with many captures, no captures ever used |
| 29 | Solid Metal | S (two schematics: a hybrid head, an all-JFET head); no captures | Valvestate 8100 (7), Head (11), Contour (38); Randall 1985 (5), 1987 (16), Satan 50 (3) | Valvestate 5.4 / 5.1 / 6.3 vs 5.1 / 4.3 / 3.4; Randall 1985 11.5 vs 6.1, 1987 4.7 (1.5) vs 4.0, Satan 3.3 vs 2.8 | **high**: also, the clean channels (all linear to -6 dBFS) are below our lowest Gain |
| 30 | Cedar Twelve | all R; no schematic, no captures | none | n/a | **no evidence at all** (a candidate for the Blues DeVille captures) |
| 31 | Triple Blend | placeholder parts, fitted to the three-amp captures at three Gain settings | | | low |
| 32 | Jazz Box | not listed in AMP_SOURCES.md (not checked) | Blues DeVille (51): this amp fits those captures best | Blues DeVille intended (Tweedle Dee / Twin Sparkle) 6.8 to 7.3 vs 2.5 to 3.2 | see note |
| 33 | Brit Eighteen | not listed in AMP_SOURCES.md (not checked) | JTM 45 (50) is mapped to it as well as Plexi | JTM 45 4.8 (0.5) vs 3.8 (shared with row 23) | see 23 |

Note on Blues DeVille (51 captures): none of the Fender-style amps has its distortion character (7.3 against 3.2 for the best of any amp, Jazz Box). That points to either a Blues DeVille-like voicing in Cedar Twelve (all R, no evidence at all today, and its "Big Spring" voicing is already a 40 W two-6L6 combo) or a new voicing for Twin Sparkle.

## 4. Where I'd put the effort (ranked)

Ranked by: how thin the evidence behind the amp is today, how large the character gap is, and how many captures there are to build from.

1. **Dual Thirty** (50 DC30 captures, schematic-only, one value assumed, tone is R). The biggest character gap with the most data.
2. **Solid Metal** (120 captures, schematics only). The Valvestate OD1/OD2 gain sweeps (G1 to G10) are the best data in the batch for getting the **gain taper and clipper character** right, and the two clean channels show a missing range (our Gain 0 is already saturating). Hybrid voicings follow the Valvestate; JFET voicings follow the Randall.
3. **Master Cranker Stock** (68 captures). The schematic-built voicing disagrees with real 2203 behaviour (8.0 against 4.6). Check which schematic value is off before changing the captures-based voicings.
4. **Slow Burn** (35 captures; Crunch and Sustain are R). The SLO100 and modded SLO100 folders: 5.0 to 7.3 against 3.6 to 4.7.
5. **Low-gain Plexi** (BJA 8.6 against 3.7; JTM 45 outside its best settings). A new voicing, existing Plex Appeal voicings untouched.
6. **Cedar Twelve / Blues DeVille voicing** (51 captures, zero evidence today).
7. **Matt's Amp against the Matamp GT1** (5.4 against 3.4); the other Matamp captures are unusable.
8. **5555 Block**: Modern to the Misha recordings, the others spread over Invective and the Peavey/EVH captures (character is already decent, 3.0 to 5.9).
9. **Wreck Stack / Power Brawl**: extra voicings from the ENGL folders; character is already mostly close.
10. **Citrus Doom** (Sunn Model T, 4.1): all R, so any evidence helps, but the fit is already reasonable.

Everything else is either close already, has no data in this batch, or you asked me to leave alone (Eclipse).

## 5. Proposed way of working for each fit

1. Start from the schematic structure if one exists; do not replace it with a capture fit.
2. Fit only **gain structure** (stage gains and taper, clipper hardness, bias, sag) to the capture sets, and leave the tone offsets alone.
3. Render a short before-and-after for you to listen to (same riff, same settings) before changing the defaults.
4. Keep every existing voicing; add or adjust only what you approve.
5. A dynamic check (bursts and decays against the NAM files) for feel, if you want it added.
