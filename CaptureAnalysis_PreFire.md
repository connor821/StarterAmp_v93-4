# PreFire captures: first pass (33 files, 11 models x Drive 0, 5, 10)

Source: `Captures/StarterAmp_TestPack/PreFire/<model>/Drive {0,5,10}.wav`, rendered from the test pack (ALL_v2), analysed with `_an_decap/an2.py` (full per-file numbers in `_an_decap/pf_summary.txt` on the Mac). All 33 files aligned and usable.

Gain at -40 dB input (small-signal gain, dB): Drive 0 is about 0 dB for every model, Drive 5 is +5 to +7, Drive 10 is +11 to +12.6. So Drive is mostly input gain; the character is in what happens above it.

## Drive 10: how hard it squashes and which harmonics it makes

Compression = gain loss of the 1 kHz burst at -10 dB peak relative to the -40 dB burst. H2/H3 = level of the 2nd and 3rd harmonic relative to the fundamental (dB) for the -10 dB burst, and the swept tone at -12 dB.

| Model | Compression @-10 | H2 / H3 (burst) | Character |
|---|---|---|---|
| Tigi | -29.8 | -4 / -13 | Hardest limiter. Even-heavy, almost flat in frequency |
| B43 | -28.6 | -4 / -11 | Very hard, even-heavy (H2 about -3 dB across the sweep) |
| Ap1 | -22.6 | -38 / -10 | Hard, symmetric (H3 only), like hard-clip diodes |
| Mida | -20.2 | -40 / -10 | Hard, symmetric (H3 only) |
| VBOX | -17.8 | -18 / -10 | Hard, both harmonics (H2 -16 / H3 -11 over the sweep) |
| Universe | -17.6 | -38 / -10 (burst); H2 -24 over the sweep | Hard; H2 present on the sweep, odd on the burst |
| N3V3 | -14.2 | -7 / -32 | Medium, very even (H2 -7, H3 -30): asymmetric clipper |
| Germanium | -11.2 | -3 / -10 | Medium, H2 almost as loud as the fundamental; bass compresses more than treble (60 Hz -14 dB vs 1 kHz -10 dB) |
| Focus | -8.0 | -32 / -12 | Mild, odd |
| Millenium | -2.3 | -29 / -16 | Barely compresses; a little of both |
| Gemel | -0.5 | -44 / -27 | Essentially clean even at Drive 10 (a transparent preamp) |

## Drive 5 (where the knee starts)

Most models are still linear at Drive 5 (compression under 1 dB at -10 dB). Exceptions: VBOX is already compressing (-6.6 dB) with H2 -12 / H3 -15, Germanium is breaking up on low notes (60 Hz H2 -17 dB, gain -1.8 dB at 60 Hz), Universe starts (H2 -21) and N3V3 starts (H2 -25).

## Tone shaping with level

At Drive 10 the pink-noise bands lose different amounts when level rises 12 dB: Germanium loses 5.7 dB at 63 Hz and 0.1 dB at 8 kHz (bass clips and sags first, highs pass), Tigi/B43 lose 12-16 dB across the board with the top band a little less, Ap1/Mida/Universe/VBOX lose about 10 dB in the lows and 5.6 to 6.8 dB at 8 kHz (a pre-clip high-frequency lift or a high-pass in the gain stage).

## What this means for our saturator types

- There are four distinct behaviours: (1) clean, wide headroom (Gemel, Millenium); (2) even-order, frequency-dependent germanium-style (Germanium, N3V3); (3) very hard even-order limiting (Tigi, B43); (4) symmetric hard clip (Ap1, Mida, Focus), with VBOX and Universe in between.
- Germanium and N3V3 are the most likely to add something we do not have: an even-harmonic clipper whose threshold depends on frequency (bass clips first).
- Ap1 and Mida are probably close to the diode clip types we already have; this should be checked in the overnight comparison against our saturator types and the Decapitator and Saturn results.
- These are single captures at each Drive; Drive 2 or 3 and a second level would show the knee better, but the three points are enough for first fits.
