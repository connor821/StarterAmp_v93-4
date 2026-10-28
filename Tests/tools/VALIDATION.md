# fitcapture validation (what was and was not tested)

Plug-in sources: snapshot of Source/ taken before the b49 change to PowerSection.h. The library was measured on that snapshot and was NOT rebuilt against the current Source; rebuild it (delete fitcapture_lib.tsv) before relying on the numbers.

## Self tests (our own amp as the capture)
Built-in-signal path (`--capture-amp`), library of 660 runs:
- On-grid Gain, rank 1 correct with error 0.00 for: Twin Sparkle V1 0.35, Modern Maul V1 0.65, Tweedle Dee V3 0.20, Matt's Amp V4 0.80, Triple Blend V1 0.50, Console DI V1 0.50, Revved Up V2 0.50. Runner-up errors 0.1 to 1.7 (several amps within 1 to 2 dB of each other at high gain, so neighbours are not well separated there).
- Off-grid Gain: without refinement the true amp was NOT rank 1 for Chime Thirty V2 0.42 and Diesel Fuel V2 0.72 (found only in the free-offset list), Stolen Fire V3 0.57 rank 2. After adding Gain refinement: Diesel Fuel V2 0.72 -> rank 1, score 0.10; Chime Thirty V2 0.42 -> rank 1, score 0.15. Stolen Fire 0.57 and Jazz Box 0.30 were run before refinement existed (Jazz Box rank 1, 0.30; Stolen Fire rank 2) and not re-run.
Recording path (`--di D_synthetic_playing.wav` arpeggio, 24 s, `--out` = our Revved Up V2 at Gain 0.50 rendered on that DI): rank 1 Revved Up V2 Gain 0.50, error 0.00 (H2 0.01); rank 2 Revved Up V1 0.48. Only 5 of 6 level bins and 3 bins with pitched notes were available, so the gain curve there rests on few points.

## Real captures (NAM, built-in signals; refine 4); intended amp = the one MODELED_GEAR.md says the plug-in amp is inspired by
Completed (8): Deluxe Reverb Head 2 (attached): best Brit Eighteen V1 Gain 0.63, score 2.14, gain 0.9 / H2 2.7 / H3 3.3 dB -> verdict "no existing amp within tolerance" (H3 just over); with a +8 dB level offset it would pass; Twin Sparkle rank 5. Deluxe 1965 clean: none within tolerance, Triple Blend / Hi-Wat Headroom first, Twin Sparkle rank 4. Deluxe 1965 cranked: none, Marque Dream V4 first at the Gain floor (needs 12+ dB less drive than Gain 0.2), H3 error 8 dB, Twin Sparkle rank 14. TwinVerb: none, Marque Five first, Twin Sparkle rank 2. Bassman G5: none (Eclipse first); G10: "close" Eclipse V2 (gain 1.2, H2 2.9, H3 1.8), the intended Tweedle Dee ranks 14. VH4 ch2 crunch: "close" Revved Up V1, Diesel Fuel rank 3. VH4 ch4: none, Plex Appeal first, Diesel Fuel rank 14.
Not finished (stopped at the coordinator's request): VH4 ch3, Mesa Dual Rectifier / Roadster / Mark IIC+ / Mark V / Single Rectifier, Prometheus. Not validated.
Reading: the tool runs end to end on real captures and the ranks are not nonsense (Fender-type captures: Twin Sparkle rank 2 to 5 on 3 of 4), but agreement with the "intended" amp is weak (rank 14 on 3 captures), so the ranks mostly show that the plug-in amps are not tight models of these captures, or that H2/H3 of these NAM models are noisy. No ground truth beyond the self tests exists, so "works" here means: finds a known amp when one is present, and reports plausible components otherwise.
DI cross-check (`--nam` + `--di`) ran on the Deluxe capture: it ranked the same amps in the same order but with larger errors (best 3.8 vs 2.1); not checked further.

## Not tested
CMake example, the stale-library check, `--only` with names, 44.1 kHz DI, Windows, `--modes`, `--quality`, `--jobs` above 2.
