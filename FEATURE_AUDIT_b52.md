# Outside audit notes and feature list, checked against b52

Two notes from outside sources (a source-level audit of b51, and a "what to build next" list) were checked against the b52 source and manual. Nothing here was listened to; "present" means found in the source, not judged to sound right.

## 1. The audit notes

The audit was done without building (it could not download JUCE), so every item was a prompt to check, not a confirmed defect.

| Audit point | What the source shows | Action |
|---|---|---|
| DI Match learning used a blocking lock on the audio thread | True. When learning finished, `processBlockChunk` took `diLock` with a blocking `ScopedLockType` to publish the learned shape. The message thread takes the same lock (state save, load, reference changes), so the audio thread could wait. | **Fixed in b52**: a try-lock, retried on the next block if busy. |
| Patch graph refresh | Already a try-lock (`refreshPlan`). The compile runs under the lock but only on a patch change; the patch is at most 96 cables. | Cost not measured. |
| NAM model hand-over | The audio thread only try-locks the inbox and swaps pointers. The retired model is destroyed on the message thread at the next install. | No change. Rapid replacement and teardown not stress-tested. |
| Oversized host blocks | `processBlock` splits blocks larger than the prepared size. | No change. |
| Buffer sizing at tiny block sizes | The block-size test renders seven amps at block sizes 1, 16, 32, 64, 100, 128, 333, 1024 and 4096 and checks they stay finite and within 1 dB of the 512 render. | No change. |
| CPU, latency, allocations, thread races, host behaviour | Not measured by this project's tests (the test README already says so). | Open: allocation counting, a ThreadSanitizer run, DAW tests, and listening comparisons. Needs a machine with the DAWs. |
| Amp models are approximations | Correct and already stated in AMP_SOURCES.md and MODELED_GEAR.md. | None. |

## 2. The feature list

**Delay engine** (the list's top item)

| Item | Status in b52 |
|---|---|
| Digital, analog (BBD), tape, reverse, diffused, lo-fi delay types | Present (types, Reverse playback, Diffusion, Lo-Fi). b52 adds Tape Sustain, Three-Head Tape, Chorus Bucket, Studio Tape, Dub Echo. |
| Ping-pong, dual, tempo sync, multi-tap, spread | Present. |
| Filtering in the feedback loop | Tone low-pass (always); b52 adds a loop high-pass in the new styles and the echo EQ with Decay. |
| Saturation per repeat | Present since earlier (mode drive); b52 adds Saturation with four curves. |
| Diffusion, ducking, freeze / infinite | Present. |
| Modulation inside the loop | Partly: the delay-time modulation affects every repeat because the loop reads at the modulated time. No separate modulation of the feedback path. |
| Swing, rush / drag, accent | **Added in b52** (Groove, Feel, Accent). |
| Pitch-shifted feedback | **Added in b53** (Repeat Pitch, Pitch Fine, Pitch Mix on both delays). |
| Per-repeat compression | **Missing.** |
| Per-repeat panning, mid-side mode, independent L/R feedback, cross-feedback beyond Ping-Pong | **Missing.** |
| MIDI clock sync | **Missing** (host tempo and tap tempo only). |
| Reorderable feedback loop (the modular loop) | **Missing**, and a large project. |

**Effects**

| Item | Status |
|---|---|
| Gate with attack, hold, release, detector filter, key choice, expander | Present (Gate block, plus the input gate, the Chug Gate and De-Noise). Hysteresis is fixed (4 dB), not adjustable. |
| Two-stage gating (before and after the amp) | Present in effect: the input gate, the Gate block (movable on the board) and the Chug Gate after the preamp. |
| Transient shaping | **Added in b53** (new panel on the Input page, in the Gate's board slot). |
| Dynamic EQ / dynamic low control | **Missing** as such. Multiband compressor and the cab's Tame (resonance suppression) exist. |
| Multiband saturation | **Done in b55**: Multiband Saturator (four bands, own splits, five curves, Drive and Level per band, 2x oversampled with ADAA). |
| Polyphonic transpose, harmonizer, momentary pitch, doubler, detune | Present (Transpose / Formant, Harmonizer with chord tracking, Pitch pedal, Octaver, Noise pedal, Doubler). |
| Ring modulator, sequenced filter / tremolo, granular, envelope-driven modulation | Present (Noise pedal ring mod, Slicer and Filter motions, Granular Cloud, envelope filter and phaser envelope). |
| Pitch-delay sequences | **Missing.** |

**Amps**

All five suggested families already have a model: Diezel VH4 (Diesel Fuel), ENGL Savage and Powerball (Wreck Stack, Power Brawl), Friedman BE (Brown Eye), Mesa Mark IIC+ / IV / V (Marque Dream, Marque Five). Only the **Bogner Uberschall** has no model (Harmonic Haze is Shiva-based). The note's own advice applies: add it only if a level-matched comparison shows a gap.

**Cabinet**

| Item | Status |
|---|---|
| Dual-mic phase and alignment (level, pan, polarity, delay, auto-align, display) | Present (four cab blocks). |
| Speaker behaviour beyond a static IR (compression, break-up, cone motion) | Present (Speaker Dynamics Real modes, Tame, Motion). Frequency-dependent impedance with the amp is not modelled. |
| Room and distance | Partly: mic Distance and a Room control. No early-reflection or room-size controls. |

**Product features**

| Item | Status |
|---|---|
| A/B with level match | Present. |
| Preset browser with tags | Present. |
| Tuner and metronome | Present (tuner with all-strings view; metronome follows the tempo). |
| MIDI learn | Present. No per-mapping range, no MIDI output. |
| Preset morphing | **Missing** (tried in b56, taken out in b57: it did not work well enough and is low priority). |
| Global locks (keep gate, transpose, output across presets) | **Present (b56), per page:** right-click a page tab to keep that page when a preset loads. Not per control. |
| Parallel-processing templates | **Partly (b56):** one button on the board adds a dry path around a pedal. No ready-made multi-pedal templates. |
| Input / output diagnostics | Partly: input and output peak meters with clip lights and a gain-reduction readout. **b53 adds a CPU and reported-latency readout** in the header. No noise-floor display. |

## 3. Suggested order (my read)

1. Transient shaper and a dynamic low-end control, which pair with the existing gates and the Tight / Focus work.
2. Global locks and preset morphing.
3. Pitch-shifted feedback for the delays (the shifter exists; the loop needs bounding).
4. Parallel templates and a small diagnostics strip (CPU, latency, noise floor).
5. An Uberschall-style amp only after a blind comparison shows a gap.
