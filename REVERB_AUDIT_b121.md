# Reverb / Cab Room audit against plug-in renders (b121 baseline)

**Measured, not listened to.** The references are *plug-in renders* (Arturia Spring, UAD Sound City, Soundtoys Little Plate, UAD Pure Plate, UAD Capitol Chambers) of the same test file, not hardware. Click removed from 58 of the 91 files first (cleaned copies in `_reverb_clean/`, originals untouched).
Method: exponential-sweep deconvolution, per-octave RT60 (EDC, t20), EDT, tail tilt (energy 200 ms–1 s vs the 1 kHz band), L/R correlation, loud-vs-quiet sweep error (modulation/non-linearity). Ours rendered via patch 3 (In → Cab off → Reverb → Out), mix 100 %, pre-delay 0, cuts open.

**Not covered:** Pure Plate 1s/2s and 35 of the 48 Sound City files (see section 3: the sample shows they are the same room). Nothing in the reverb types has been retuned (b122 only added the Big room cab voicings). Everything below is a proposal for your review.

## 1. Plates (Plate 3, Soft Plate 7, Studio 78 18) vs Soundtoys / Pure Plate
| Finding | Ours | Reference |
|---|---|---|
| Onset gap | Plate and Soft Plate 10–21 ms, grows with Size | 0 ms (Studio 78 also 0) |
| Decay vs Size knob | about 82–85 % of the set value | matches the set value |
| Brightness at default Damp 0.5 | 8 kHz RT 0.3–0.4 s, tilt about −48 dB | 8 kHz RT 1.5–2.3 s, tilt −16 to −27 dB |
| Brightness at Damp 0 | tilt −28 dB, still dark | |
| Modulation | always time-varying (loud/quiet error about +2.5 dB), even at Mod 0 | "Mod Off" exactly static |
| Full-wet level | 8–10 dB below dry | about unity |
Proposals: remove the onset gap (as Studio 78 does); make Size read true decay; open the default top end by roughly 20 dB of 8 kHz tilt; make Mod 0 truly static; raise full-wet level about 8–10 dB (or compensate in the Mix law, not by a new knob).

## 2. Chambers: Capitol Rooms 2/4/6/7 vs our Classic / Room / Hall / Ambient / Cathedral
Reference: RT60 2.6–4.7 s, 4 kHz RT 1.7–2.2 s, 8 kHz RT 0.8–1.4 s, 8 kHz tilt −22 to −37 dB, L/R correlation about 0 (fully decorrelated), static (no modulation).
Ours at Size 2.6 and 4 s, Damp 0.5:
- **Classic (0)** is the closest: 4 kHz RT 1.7–2.3 s, 8 kHz RT 1.1–1.3 s, 8 kHz tilt −15/−16 dB, correlation about 0, static. It is a little too bright at the very top.
- **Room (1), Hall (2), Cathedral (5)** are far too dark: 4 kHz RT 0.6–0.8 s, 8 kHz RT 0.3–0.4 s, 8 kHz tilt −45 to −64 dB. Their early L/R correlation is 0.3–0.6 against about 0 for the chambers, and they carry the always-on modulation (about +2.5 dB).
- **Ambient (4)**: 4 kHz RT 0.8–1.0 s, 8 kHz tilt −43 to −46 dB, and a 28–34 ms onset gap.
Proposal: Room/Hall/Cathedral want about 1.5 s more 4 kHz decay and roughly 25–30 dB less HF tilt; decorrelate their early stereo; offer a static option.

## 3. Cab Room vs Sound City 4x12 tight Room mics
Reference Room mics: RT60 about 0.45 s, flat across bands (8 kHz about 0.3–0.4 s). Room 1 mics are bass-heavy (125 Hz tilt +25 to +32 dB, 8 kHz −20 to −22 dB); Room 2 mics are flatter (+8 dB at 125 Hz, 8 kHz −37 dB). Close mics have no tail.
Ours (cabRoom 1.0; 0.5 gives about 6 dB less room-to-direct):
| Type | t20 | 125 Hz RT | 8 kHz RT | 125 Hz tilt | 8 kHz tilt |
|---|---|---|---|---|---|
| 0 | 0.15 s | 0.07 | 0.16 | +30 | −29 |
| 1 | 0.24 s | 0.34 | 0.17 | +21 | −23 |
| 2 | 0.25 s | 0.37 | 0.19 | +21 | −23 |
| 3 | 0.43 s | 0.55 | 0.33 | +20 | −21 |
- Type 3 matches the reference length (0.43 vs 0.45 s) and its 8 kHz tilt matches Room 1 mics. Types 0–2 are about half the reference length, and type 0 is effectively early reflections only (125 Hz RT 0.07 s).
- The bass-heavy tilt of the Room 1 mics is reproduced (+20 to +30 dB). No type resembles the flatter Room 2 mics.
- The 8 kHz RT in ours (0.17–0.33 s) is a little short against 0.3–0.4 s.
- Room-to-direct level could not be compared (the reference Room and Close files are separate mics with unknown relative gain).
Proposals: lengthen types 1 and 2 toward 0.4–0.45 s (125 Hz RT too); keep type 3 as is; add a flatter, less bass-heavy voicing only if you want the Room 2 mic character, as an extra voicing, not a replacement.

## 3b. Springs: the 16 Arturia renders vs Spring (6) and Spring (tank, 17)
Reference (Accu 4/8/9, Accu vintage, Gibbs Alt/Orig, Space Echo, Synthi; Short and Long each):
- Decay: Short about 0.85-1.1 s, Long about 2.5-4.8 s. Fairly flat to 4 kHz (4 kHz RT 0.7-2.4 s), 8 kHz RT 0.5-1.8 s.
- Top end varies a lot by model: Gibbs is bright (4 kHz tail tilt 0 to -10 dB, 8 kHz -28 to -35), Vintage, Space Echo and Synthi are middle (-17 to -29 / -43 to -52), Accu 4/8/9 are dark (-22 to -30 / -56 to -68).
- **Mono** (L/R correlation 1.00 on every file).
- Mildly level-dependent: loud and quiet sweeps differ by -8 to -20 dB error and the loud one is 0.4-2.7 dB quieter (soft compression), but it is not time-varying.
Ours:
- **Spring (6):** decay is right (0.85 s at Size 1, 3.7 s at Size 4.5), but it is too dark (4 kHz RT 0.24-0.56 s, 8 kHz RT 0.14-0.24 s; tilt -37 to -57 / -56 to -75), stereo (correlation 0.3-0.6), and time-varying (loud/quiet error +0.3 to +1.5 dB).
- **Spring (tank, 17):** at Damp 0 it matches Vintage, Space Echo and Synthi on top end (tilt -18 / -44 to -45) and Size 1 / 4.5 give 0.99 / 4.3 s. At the Damp 0.5 default it is much darker (-26 / -58 to -62). It is static and perfectly linear (no level dependence), stereo (0.6-0.7), and has a 28 ms onset gap.
- Nothing reaches the Gibbs brightness.
Proposals: default Spring (tank) Damp to about 0.1 (it covers four of the seven spring models without any new control); extend the Damp range so Gibbs-style brightness and Accu-style darkness are both reachable; default spring Width to mono (the Width control exists); add a small level-dependent compression to the tank (about 1-2 dB at hot levels); remove the 28 ms onset gap if it is not part of the spring's character; make Spring (6) static or give it a Mod 0.

## 3c. Sound City: are the other 35 files worth copying?
Sample of 5 more (4x12 live U67 R1, 2x12 live U67 R1, 2x12 tight U67 R1, 4x12 tight C24 R1, 2x12 live KM54 R2) against the 4 already measured: decay bands stay at 0.49-0.58 / 0.39-0.47 / 0.43-0.52 s (low / mid / high), live is about 0.05 s longer than tight, and the mic only moves the bass-to-mid balance by a few dB (C24 is bass-heavy like KM84 Room 2). It is one room. The three Big room voicings already span that, so I stopped copying. More room options need different rooms.

## 4. Priority
1. Room/Hall/Cathedral/Plate top end (largest gap, 20–30 dB of HF tilt).
2. Plate onset gap and Size-to-decay accuracy.
3. Static Mod 0 / always-modulated tail.
4. Full-wet level on plates.
5. Cab Room types 1–2 length.
6. Springs: Damp default and range on Spring (tank), mono default, mild compression (section 3b).

## 5. What b123 changed (measured after the change, same method)
| | Before (Damp 0.5, old default) | After (Damp -0.2 default / reachable) | Reference |
|---|---|---|---|
| Plate 8 kHz RT / tilt | 0.39 s / -47 dB | 0.65-0.83 s / -21 to -27 dB (Damp 0 to -0.3) | 1.5-2.3 s / -16 to -27 dB |
| Soft Plate 8 kHz RT / tilt | 0.31 s / -59 dB | 0.53-0.84 s / -19 to -33 dB | same |
| Plate onset | 10-21 ms late | 0 ms | 0 ms |
| Room / Hall / Cathedral 8 kHz tilt (at -0.5) | -56 to -63 dB | -16 to -20 dB | chambers -22 to -37 dB |
| Spring (tank) 8 kHz tilt at Damp -1 / 0.1 / 0.5 | -58 dB at 0.5 | -24 / -46 / -58 dB | -28 (Gibbs) to -68 (Accu) |
Plates top out near Damp -0.3 (the loop filter reaches its cap), so -0.3 to -1 sound the same there; the extra range is for Hall, Cathedral and the spring.
Still open: static Mod 0, plate full-wet level, spring compression, Pure Plate 1s/2s.
