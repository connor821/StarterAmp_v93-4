# Capture comparison (written against b62, report only: nothing in the plugin was changed)

Brand names appear here because this is a docs file; none of them are in the UI or source.

## How it was measured (and what it can't tell you)

- 1,205 NAM captures (60 folders plus 11 loose files) were run through the NAM runner: gain at three levels, an 8-band spectrum profile (63 Hz to 8 kHz, relative to 1 kHz), and a 196 Hz sine at five levels for compression and 2nd/3rd harmonic.
- Our 31 amp models were rendered the same way: 744 renders (model x voicing x Gain 0/2/4/6/8/10, default tone and level, no cab).
- Score = spectrum error + 0.15 x compression error + 0.15 x harmonic error. For scale: the captures we fitted in earlier builds score 3.6 to 3.8 (Digital Dread) and about 2 to 3 (Diesel Fuel). **2 or less is a very close match, up to about 4 is "fitted quality", 5 or more is a different voice.**
- Limits you should know about:
  - Our tone knobs were at default and the captures' knob settings are unknown, so a folder's best capture is the fair number; the median is pessimistic.
  - It is one spectrum, one tone, and a harmonic ratio. It does not hear dynamics, feel, or sag. A low score says "similar on these measurements", not "sounds the same". Nothing was listened to.
  - The scores are the best of about 744 renders, so an amp that has fewer voicings or gain steps in the pool has a disadvantage by construction. That is a real limit of our coverage, but part of the gap is the size of the pool.

## 1. Fit of our existing amps to the captures of their namesakes

"Folder best" = the single best capture in the folder against the intended amp (any voicing, any Gain). "Any amp" = the best of all our amps for the same folder.

### Already close (score 3 or below)

| Captures (folder, count) | Our amp | Folder best | Any amp |
|---|---|---|---|
| Mesa Badlander Rectifier (56) | Modern Maul | 1.1 | 1.1 |
| Sunn Model T (11) | Citrus Doom | 1.9 | 1.9 |
| Peavey 6505 1992 original (51) | 5555 Block | 1.7 | 0.7 |
| Vox AC30, both folders (70) | Chime Thirty | 1.8 to 2.5 | 1.6 |
| Friedman BE-100 DLX (3) | Brown Eye | 2.0 | 1.8 |
| Rivera M100 OD (35) | Marque Five (no sibling; just the nearest) | 2.1 | 1.6 |
| Soldano SLO100 (6) | Slow Burn | 2.2 | 1.5 |
| Marshall 1959 Slash (51) and Super Lead (3) | Plex Appeal | 2.7 and 2.8 | 1.4 |
| Peavey 6505 (0.5.2) (95) | 5555 Block | 2.7 | 0.7 |
| JCM 800 2203 (30) | Master Cranker | 2.8 | 2.2 |
| Matchless DC30 clone (50) | Dual Thirty | 2.9 | 1.6 |
| Fender Blues Deluxe Reissue (7) | Tweedle Dee | 2.9 | 1.5 |
| Bogner Shiva (5 and 12) | Harmonic Haze | 2.9 to 3.0 | 1.7 |
| Friedman Jose Teaser (3) | Master Cranker, fitted to these (Modded) | 3.6 | 3.5 (still fits) |

### Middling (3 to 5)

Peavey 5150 II (3.2) and Blockletter(1) (3.2), Marshall Plexi Super Lead 1959 EL34 (3.5), Randall RG100ES 1987 (3.5), Synergy SYN-50 (3.5, no sibling), Fender Tweed Deluxe 57 (3.6), Ceriatone Molecular (3.9 on Plex Appeal), ENGL Savage 120 MK1 (3.9) and MKII (3.4), Fender Blues DeVille 4x10 (4.1), Valvestate 8100 Head (4.1) and 8100 (4.2), EVH Stealth Red (4.4), SLO100 modded (4.5), Fireball Pack (4.3), mk I higain (4.7), Bogner Uberschall (4.8), Peavey Invective (4.9), SLO30 (4.8), 5150 Stealth preamp (4.8), JCM800 Zakk (4.9).

### Poor (5 or more): where our voicing is not close to the capture

| Captures | Our amp | Folder best | Any amp | Note |
|---|---|---|---|---|
| Peavey Invective 120 (1) (4) | 5555 Block | 9.4 | 2.7 | The R voicings are the issue |
| Matamp GT100 / GT1 (9) | Hi-Wat Headroom (nearest clean 100 W) | 7.4 to 8.5 | 2.7 to 5.9 | Different voice from the Hiwatt-style stack |
| Marshall 1959BJA (11) | Club Crunch | 7.3 | 1.2 | |
| Fender Blues DeVille 1994 DI (11) | Twin Sparkle | 7.3 | 2.5 | |
| Marshall JTM 45 (50) | Plex Appeal | 6.9 | 2.9 | Low-gain, open lows; our Plexi has no such voicing |
| Orange OR120 clone (3) | Matt's Amp / Marmalade | 6.6 | 3.7 | |
| Blockletter (4) | 5555 Block | 6.6 | 2.8 | |
| Valvestate + Contour (38) | Solid Metal | 6.4 | 3.2 | |
| ENGL Fireball 100 full gain (10) | Power Brawl | 6.3 | 2.1 | |
| Randall Satan 50 (3) | Master Cranker | 6.2 | 2.9 | |
| Soldano SLO LTD Teaser (2) | Slow Burn | 6.0 | 2.2 | |
| Friedman Jose Teaser, under Brown Eye | Brown Eye | 5.6 | 3.6 | Belongs under Master Cranker (above) |
| Powerball II (8) | Power Brawl | 5.4 | 2.3 | |
| Ironball (14) | Power Brawl | 5.3 | 2.0 | |
| EVH 5150iii Ivory (28) | 5555 Block | 5.1 | 1.9 | |

### What this says

1. **5555 Block's Modern, Crunch and Buzz (all R) never win against any 5150/6505 capture**; only Raw (the S-part voicing) does. There are about 270 Peavey/EVH captures to fit them to. This is the largest unfitted body of captures.
2. **Wreck Stack (all voicings R) never wins against any ENGL capture**; Power Brawl does a little. About 60 ENGL captures.
3. **The Plexi is good in its cranked form and weak at low gain** (JTM 45 and 1959BJA at 7 or more). A low-gain Plexi voicing would cover those.
4. **Valvestate, Matamp, Orange and Randall** are small, distinct sounds with no real sibling or an R-only one.
5. Many "intended" best fits sit at the top of our Gain range (Chime Thirty, Harmonic Haze, Master Cranker, Plex Appeal, Matchless, Power Brawl, Solid Metal). Hint only, since the captures' knob settings are unknown: more gain range at the top would not hurt (extra range is always better).
6. The existing C-fitted voicings still hold (Modern Maul on the Mesa captures 1.1, Citrus Doom on Sunn 1.9, Chime Thirty on the AC30 captures about 2, Master Cranker on the Jose captures 3.6).
7. Modern Maul wins often because it is a generic scooped high-gain shape; "Modern Maul is the best fit" for a non-Mesa folder is not a recommendation to use it.

## 2. Boosted captures

Two kinds: ones the file name or message says are boosted (38 real pedal boosts), and unlabelled suspects found from the data.

**Labelled as boosted (pedal in front):** ENGL Savage RED RHYTHM TS808 1 and 2, Savage 120 Crunch I and II + Fortin TS808, Fireball + TS9 / + TS808 / +TS9, VA Merciless Drive (2), Peavey Invective Lead Gain 5 + TS808, Blockletter red od808 / od808x (hg and lg), green od808x, Blackletter Green + TS9 and Red + TS9, Ceriatone Molecular HI Gain #10, #14, #15 (TS boost), Badlander Bold Crush Astro Horsey #01 and #02 (TS), Marshall 8100 OD2 with Boss MT-2 boost, Bogner Shiva SLP MED BOOST, JTM 45 treble-booster P2 and P4, Marshall 1959 Plexi Super Lead + Boost.

**Not pedal boosts, despite the name:** "unboosted" and "no boost" files, the AC30 "TopBoost" files (that is the Top Boost channel), Valvestate "CH Boost OD1/OD2" (channel names), and Randall "Midboost" (the amp's own circuit).

**What they look like:** compared with the other captures in the same folder, the TS-boosted ones have tighter lows (63 and 125 Hz together about 5 to 12 dB further below 1 kHz in Ceriatone, Savage mk I, Badlander and Invective) and, in most folders, the same or somewhat higher gain. They also tend to fit our high-gain amps better (in 12 of the 15 folders with both kinds the boosted average is lower, by up to 2.1 points, for example Savage mk I 1.9 against 4.0 and Badlander 1.3 against 3.1), plausibly because our high-gain amps already have a tight front end. The Fireball + TS captures are the exception: no extra tightness.

**Unlabelled suspects (tight lows and high gain, relative to their folder):** Marshall Law RAW (Headbangers pack), Rivera "1 Metal Modify" (lows 56 dB under the mids, the tightest of all 1,205), Ceriatone "Clean #02" (lows 27 dB down, odd for a clean capture), JTM 45 "V5 Locut P5" (probably just its own low cut). Treat these as suggestions.

**What I'd do with it:** nothing is fitted to a boosted capture yet. If a voicing is fitted to a folder, leave the TS-labelled files out of that fit (or fit them as "Boosted" voicings), otherwise the voicing comes out too tight for the unboosted knob settings.

## 3. Compressor captures against our styles and saturation options

The capture models are nearly static: measured attack under 5 ms and release of 0 to 125 ms are at the edge of what the test can resolve, so the real value in them is the static curve and the saturation. Dynamics should come from our own detector (which already behaves like the hardware: FET release about 280 to 400 ms at heavy compression).

| Capture | Low-level gain | Slope above knee | H2 / H3 at -12 and -6 dBFS | Takeaway |
|---|---|---|---|---|
| Warm Audio WA76D (25, ratio 4 to ALL, input 37 to 100) | -5 to +3 dB | 0.2 to 0.69 (0.41 to 0.5 for most) | about -35 to -48 dB, both | Clean. Our FET style at slopes 0.43 to 0.98 covers it; the "ALL buttons" ones are harder (0.19 to 0.27) |
| 1176LN, input sweep Inp0, 6, 12, 18, 24, 30+ | +12, +12, +8, +5, +2, 0 | 0.36 to 1.0 | **Inp0 / Inp6: H2 -20 / H3 -15 at -12, -10 / -17 at -6**; Inp12: -39 / -28; Inp18: -50 / -34; Inp24 and up: clean | The input knob is a saturation knob: low-level gain, slope and even+odd harmonics all move together |
| Neve 1176 (loose file) | +37 dB at -48 | 0.09 | H3 -26 / -27 | Almost a hard limiter with fuzz |
| Distressor (3 files) | +6 and 0 | 0.44 to 1.0 | "red line" H2 -35 / H3 -12; the Saturation capture is clean | One useful: the red-line capture. The `distressor.nam` file is unstable (H2 about 0 dB): ignore |
| Zero Chill (6) | +4 to +11 dB | 0.4 to 0.6 | below -38 dB | Clean modern compression; smooth, low makeup tilt. Nothing our styles can't do |
| UREI 1176 L / R and SF Fast | 0 to +2 | 0.7 to 0.9 | below -37 | Barely compressing at the test level; not informative |

**Our options, measured the same way** (FET style, sat 0 to 8): Tube-like H2 -18 / H3 -23 at -12 dBFS; Tape-like -33 / -20; FET stage -34 / -18; sat 6: -18 / -25; Diode: +5.1 dB low-level gain, H3 -18. Crush Limiter: +11 to +37 dB at low level depending on Crush, slope 0.0 to 0.68, H3 -11 with Crunch up and H2 -30.

**Gaps:** none of ours has the 1176 input-slam signature (H2 about -20 and H3 about -15 *together*, +12 dB at low level, 0.36 slope). Tube-like is the closest at H2 -18 / H3 -23 but it is softer on H3. A new saturation entry fitted to Inp0, Inp6, Inp12 (a three-point curve) would be the useful addition, and the saturation options all stay as they are ("keep them as options").

### "1073 + 1176 as its own distorted DI thing"

Makes sense and fits the current structure: the Console DI model (1073-style) with its transformer colour in front, then the compressor in FET style with the input-slam saturation. The 1073 stage gives soft, even-weighted colour and the slam stage adds the H3 and the squash, which is what a pushed console into an 1176 does on bass and DI guitar. Suggested form: one extra Console DI voicing (or a "DI slam" mode), the Inp0 to Inp12 curve driven by one knob, no new panel. I haven't built it; say if you want it in b63.

## 4. Tascam Portastudio 424 captures (the capture side of #189; the manuals and Level-Loc schematic are still to read)

Five captures (Distortion folder: three; Preamp Distortion folder: two). They are hard-clipping, high-gain preamp distortion:

- +17 to +39 dB gain at low level, falling to about 0 to -8 dB at 0 dBFS, slope 0.0 to 0.09 (a limiter, effectively).
- **H3 about -10 dB at -12 and at -6 dBFS** (so the 3rd harmonic is already at a third of the amplitude of the fundamental, nearly a square wave); H2 -20 to -41.
- Our Tape boost mode and Cassette Lo-Fi voicing are soft saturation, so they are not close to this. This is a different sound, not a setting of the same one: it needs its own entry (a hard-clip preamp stage with a Baxandall tone pair, which the Tascam, and the Synapse Burnout pedal below, both have).

## 5. Chimera Mechanical (the products page and two product pages)

Only the Zero Day and Synapse Burnout pages carry detail; the others are listed with a name, type and price.

- **Zero Day, "Glitch Generator", time-based fuzz.** Two controls are described. **Warp**: counter-clockwise gives a quick double-tracking fuzz with artifacts and stutters; clockwise it turns into a detuned mess; the further, the slower the glitch. **Glitch switch**: stutters the playing by freezing it in segments of time (latching and momentary versions). The page says nothing about the mechanism.
  - How it could map onto what we have: a short, modulated delay line (double-tracker) feeding a fuzz stage = Warp, with the modulation getting slower and deeper as it goes up; the Glitch switch is a freeze/stutter (we already have delay HOLD and the freeze/granular parts to build it from). It would sit as a new fuzz style or a Mod FX type rather than a new panel. Not built; I'd put it on the b63 list if you want it.
  - Only two controls means it fits the "simple on the surface" rule.
- **Synapse Burnout, 4-track preamp / fuzz.** Gain (low = the overdriven crackle of a 4-track while boosting well above unity; high = "high clarity vintage fuzz"), Baxandall High and Low, a low-end toggle, Volume. It is the same circuit family as the Tascam captures above, so the Tascam analysis applies to it too.
- **Debugging, "Resonant Feedback Generator"** (price only on the products page): from my earlier notes, two series delays plus a Drone ramp; the product page was not read in this pass.
- **Others listed:** Breach Protocol (bass overdriver), System Collapse (overdriver / distortion / saturation machine), Blue Screen (dimensional chorus / phase shifter), Meowler (automatic pitch modulation, sold out), Motion Blur (double tracker / envelope-controlled chorus, coming), Reboot Optics (dirty boost / overdriver, coming).

## 6. Suggested order (all optional, nothing started)

1. Fit 5555 Block's Modern, Crunch and Buzz to the Peavey 5150/6505 folders (about 270 captures, the largest unfitted group), leaving out the TS-labelled files.
2. Fit Wreck Stack and Power Brawl to the ENGL folders (about 60).
3. A low-gain Plexi voicing (JTM 45 / 1959BJA, score 7 now).
4. The 1176 input-slam saturation entry and the "DI slam" voicing.
5. A hard-clip 4-track preamp entry for the Tascam sound.
6. Zero Day-style glitch fuzz.
7. Smaller: Valvestate, Matamp and Blues DeVille voicings, if you want those sounds.

The `_nam_batch` folder in StarterAmp_TestPack on your Mac was only for this analysis and can be deleted.
