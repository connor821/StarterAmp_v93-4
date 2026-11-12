# Second-harmonic gap: what I measured (b91)

**Measured with the fit tool (a 196 Hz sine at six input levels, amp only, cab off), not listened to.** H2 and H3 are in dB relative to the fundamental, averaged over the six input levels. The capture values come from the capture-curve files made when the captures were fitted. Nothing in the engine was changed in b91 for this.

## 1. The gap depends on which capture you compare with

The b87 and b89 changelog lines said "our amps make 20 to 40 dB more H2 than your captures". That was too broad. It was true for one set of captures only.

| Amp (Hybrid, Gain 0.65) | Capture set | Capture H2 | Amp H2 | Gap |
|---|---|---|---|---|
| 5555 Block, Modern | Misha, gain 7 and Max | -31 to -32 | -32.2 | about 0 dB |
| 5555 Block, Vitriol | Invective, Gain 5 and 10 | -32 to -33 | -27.4 | about 5 dB |
| Ironside | Laney Normal and Clipped | -50 to -51 | -33.0 | about 17 dB |
| Hi-Wat Headroom, Normal | Pink Hiwatt Normal | -71 to -76 | -27.0 | 44 to 49 dB |

The 20 to 40 dB figure came from the last two rows. For the 5555 Block, the largest part of the library work, there is almost no gap.

## 2. The Hiwatt captures' H2 may be a model-error floor, not the amp

In the Pink Hiwatt Normal capture H2 rises only from -81 to -64 dB while the input rises by 40 dB. A real even-order distortion rises about 1 dB per dB (ours rises 36 dB over the same range at Gain 0.35). A level-independent -60 to -80 dB is what a neural-model error floor looks like. **This is a hypothesis, not a proof:** I did not test the capture itself. If it is right, those captures cannot say how much H2 the real amp makes, and the gap in the last row is not a fault of our amp. The Laney captures do rise like real distortion (-67 to -35 dB), and they are the better reference for the British heads.

## 3. Where our H2 comes from (knob-by-knob, Gain 0.65, mean H2 in dB; lower = less)

| Change | Hi-Wat Normal | Ironside | 5555 Modern | 5555 Vitriol |
|---|---|---|---|---|
| Hybrid (as shipped) | -27.0 | -33.0 | -32.2 | -27.4 |
| Classic mode | -41.0 | -41.8 | -36.7 | -22.2 |
| Circuit-derived mode | -20.1 | -15.7 | -23.6 | -17.0 |
| Preamp tube 12AY7 | -38.9 | -38.1 | -45.6 | -37.4 |
| Preamp tube 12AT7 | -29.1 | -32.4 | -33.6 | -25.6 |
| Bias hot (1.0) | -33.2 | -37.1 | -35.8 | -33.2 |
| Power Amp Drive 1.0 | -32.6 | -35.3 | -36.9 | -27.8 |
| Supply sag 0 or 1, Feel, power tube | within 2 dB | within 2 dB | within 1 dB | within 1 dB |

- The preamp stage shaping is the main source: Classic and Circuit-derived differ by 5 to 26 dB depending on the amp (Ironside the most, Vitriol the least); the 12AY7 tube lowers H2 by 5 to 13 dB. Hybrid falls between them for Hi-Wat, Ironside and Modern (closer to Circuit-derived on Hi-Wat, closer to Classic on the other two); for Vitriol Hybrid is lower than both.
- The power section and supply barely move H2 (Power Amp Drive lowers it by up to 6 dB, which fits push-pull cancellation).
- No knob brings Hi-Wat near the Pink capture's level; Classic mode brings Ironside to within about 9 dB of the Laney captures.

## 4. What this does not show

- It does not isolate individual preamp stages. The engine has no per-stage tap, so I can say "the preamp shaping", not "stage 2". That would take a measurement build with taps.
- H3 is a separate mismatch: at Gain 0.8 the 5555 Modern averages -20 dB against the Misha captures' -24 dB, and reaches -12 dB at the hottest input levels; Vitriol averages -11.6 dB against Invective's -25 dB. The fit matches the gain curve, not the harmonic mix.
- A single sine is not playing. Intermodulation and transient behaviour were not measured.

## 5. Options (nothing done)

1. Leave as is. The 5555 Block is close, and the Hi-Wat reference may be unreliable.
2. Reduce the stage asymmetry in Hybrid for the British heads only (Hi-Wat Headroom, Ironside), aiming at the Laney captures: needs your listening.
3. Add per-stage taps to the fit tool and repeat this on one amp, to find the stage.


## Addendum b92

- **What was done.** Per-amp Hybrid share: Hi-Wat Headroom 0.20, Ironside 0.25 (all other amps 0.50). H2 at Gain 0.65 dropped by about 11 dB (Hi-Wat Normal) and 8 dB (Ironside Treble); H3 and level held. Designed and measured, not listened to. Triple Blend untouched.
- **Provenance caveat.** The b86 note describes the Pink Hiwatt and Laney files as "plug-in bounces of the test pack", not necessarily NAM captures of hardware. Section 2 above calls the Pink difference a "model-error floor" hypothesis; that is only a hypothesis, and if those files are plug-in renders their very low H2 tells us about that plug-in, not about the real amp.
- **Schematic check.** The DR103 preamp is three ordinary asymmetric ECC83 stages plus a long-tail splitter and push-pull EL34 with feedback. Push-pull cancels even harmonics in the power stage, and feedback lowers them further, but the preamp stages should still leave some H2. A real amp with H3 near -27 dB and H2 near -70 dB would be unusual.
- **Recommendation.** Another source of captures is the best way to settle this. The most useful would be real-hardware DI or re-amp sine and sweep captures at several levels with the cab confirmed off, ideally of a DR103 (or similar) and of the Laney, from a capturer other than the one behind the current files. Until then 0.20/0.25 is a compromise, not a fit.
