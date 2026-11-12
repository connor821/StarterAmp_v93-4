# 5555 Block against the Misha Amp 2 captures, the Invective NAMs and the Peavey / EVH NAMs (analysis, then built in b88)

All numbers are measured, not listened to. The first sections are the analysis before the change; the last section says what b88 built. The library of our amps is the b85/b86 one (5555 Block is unchanged since then).
Library Gain range is 0.2–0.8, so a match "at 0.80" may be pegged and the real gap larger.

## Misha Amp 2 (16 WAVs; 14 analysed, the two "noon_copy" duplicates skipped)
- The files are the test pack played through the Archetype, stereo with identical channels. Section A (first 97.6 s) was analysed.
- Gain 0 is silent (-133 dB); Gain 3 is saturated except at the lowest input level; Gain noon/7/Max are fully saturated at every sweep level (output level does not move with input level, slope -1 dB/dB).
- H3 sits at about -24 dBc on every file, H2 at -28 to -34 dBc (Gain 3: -38 to -41 at low level, rising to -21 at high level in Voice 1).
- Saturation / harmonics: 5555 Block Raw, Modern and Buzz all match to 1.0–1.6 (weighted score) at our Gain 0.6–0.8; Voice 1 and 2 Gain 3 match at our Gain 0.4–0.5 (score 2.5–3.2, H2 off by about 7 dB: the capture is much cleaner at low level). Crunch (V3) is worst. This check does not separate the voicings.
- Tone (pink noise at -18 dB, octave bands re 1 kHz, Voice 1 noon Gain 7): capture 63 -14.1, 125 -7.0, 250 -7.1, 500 -5.1, 2k +3.5, 4k +4.2, 8k +1.7, 12k +0.2.
  Ours at default knobs, Gain 0.7: Raw 63 -21.9 ... 8k -8.0, 12k -16.6 (rms error 7.3 dB); Modern 63 -20.9, 125 -13.9 ... 8k -11.6, 12k -21.0 (9.1 dB); Buzz 8.7 dB.
  With Treble +12 and Bass +6 (the edge of the grid) the rms error drops to Raw 2.6, Modern 4.0, Buzz 4.3.
- Reading: Modern is darker and tighter than the Misha captures. The captures have no top-end roll-off at all (they look like a cab-off capture; unconfirmed). Raw is closer in tone than Modern.

## Invective NAMs (15 files, Gain 1–10, +TS808, and four full-chain presets)
- Nothing is within tolerance (best scores 2.5–3.0; tolerance gain < 2, H2/H3 < 3 dB; H2 error is at least 3.4 dB everywhere).
- 5555 Block ranks: at Gain 4 and above (and the four full-chain presets) Raw 18th–28th, Modern 6th–19th, Buzz 2nd–8th.
- Low-gain captures (1–3) are won by Ironside / Revved Up / Digital Dread; high-gain by Solid Metal (Hybrid + JFET, Hybrid), Marque One Lead, Marque Dream Lead.

## Peavey / EVH NAMs (27 files: 5150 Blockletter Red/Green, EVH 5150 III Ivory Red I–X and Blue VII, EVH Stealth, 6505 Red High, 5150 II Lead)
- 5555 Block Buzz (V4) is in the top 4 on 12 of the 27 and 1st on two (Ivory Red I, 6505 Red High Gain 2).
- 5555 Modern (V2) is top 5 on only 5150 II Lead (Gain 8), 6505 Red High Gain 4.5 and EVH Stealth Gain 9.
- Marque Dream Lead (V5) and Solid Metal win most of the rest; Clarion wins 5150 II Lead Gain 10 (1.4) and 6505 Red Gain 4.5.

## Built in b88 (measured against the captures, not listened to)
1. **Modern retuned (tone only, gain unchanged):** pink-noise fit to the Misha Voice 1 / 2 Gain 7 and Max mean, rms 0.24 dB (was about 9 dB). Needs bass +9 dB (corner x1.75), treble +6.7 dB, high mid +1.2 dB, high-pass x0.79, speaker-less roll-off 15.8 kHz. Check against other Misha gains: 2.7 / 1.0 / 0.7 dB rms at Gain 0.45 / 0.7 / 1.0. Bright and Scooped variants not calibrated.
2. **Fifth voicing "Vitriol"** from the Invective Gain 1/2/3/5/8/10 sweeps (knob 0.1/0.2/0.3/0.5/0.8/1.0): gain within about 2.3 dB, tone about 1.1 dB rms, speaker-less roll-off 18 kHz, inter-stage 12 kHz. Low-end fit hit its bound (high-pass x0.35 of Modern). H2 not matched (capture lower).
3. **Open:** cab-off assumption unconfirmed; the Peavey / EVH and four Invective full-chain presets were used for ranking only, not fitted.

## b89 follow-up
Modern was eased back halfway between the b87 sound and the b88 fit (bass +4.5 dB, treble +3.4 dB, high-pass x0.89, speaker-less roll-off 9 kHz), at your request. It now sits between the two on purpose and is no longer a fit to the Misha captures.
