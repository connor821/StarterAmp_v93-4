# Empirical Labs Distressor: manual notes (modelling numbers)

Source read: `Distressor.txt` (pdftotext -layout of the 19-page Manualslib copy, "Rev19"; the same document as `reference/compressors/sources/Distressor.pdf`). All 1028 lines were read. Page numbers below are the manual's own numbers (1 to 14).

Reading notes
- The layout extraction interleaves the right-hand column ("Using the Distressor first time", "Ratios and their Curves", "Customer comments") into the left column on pages 3 to 14; I untangled it by hand. A few right-column words are cut off at the page edge and are not recoverable (marked (?)).
- The manual is a user manual, not a service manual: **there is no schematic, no component values, no threshold level in dBu, no gain range in dB, no knee width in dB except the 2:1 figure, and no attack/release time table per knob position**. What it does give is below. Anything not in this file is not in the manual.
- "Dist 1" does not exist in this manual. The three audio distortion states are called **Normal (Clean)**, **Dist 2** and **Dist 3** (page 1, 2, 5). If "Dist 1 emphasis" meant the detector band emphasis, that is covered in section 4.

## 1. Controls and states

| Item | Value (page) |
|---|---|
| Knobs | INPUT, ATTACK, RELEASE, OUTPUT; four "huge" knobs numbered 0 to 10 with a half-step past 10 ("They also go to 10 1/2", p1). Starting point "5 5 5 5" with 6:1 (p2) |
| Settings count | "384 possible settings (not counting knob settings)" (p1) = 8 ratios x 6 audio states x 8 detector states (8 x 6 x 8 = 384; the manual does not show the product) |
| Ratio button | steps through **1:1, 2:1, 3:1, 4:1, 6:1, 10:1 "Opto", 20:1, Nuke**, then wraps (p2, p4, recall sheet p7). "Eight unique curves" (p1). Yellow LED = 6:1 (p2) |
| Audio button | 6 states in order: Norm (no LED); HP (green); Dist 2 (yellow); Dist 2 + HP; Dist 3 (red); Dist 3 + HP (p5) |
| Detector button | text says "eight states"; the list on p6 gives seven (Norm; HP green; Band Emphasis yellow; HP + Band Emphasis; Stereo Link red; Link + HP; Link + Band Emphasis); the eighth (Link + HP + Band Emphasis) is cut off in the extraction (?) |
| Bypass | hard-wired relay, true bypass; "can cause audio clicking" (p10) |
| Gain reduction meter | LED bargraph, legend on the recall sheet (p7) reads, left to right: 26, 23, 20, 17, 14, 12, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 dB (16 LEDs), plus a BY-PASS LED. "Deflects much faster than the old VU's" (p4). Resolution is high enough that 1/10 dB differences can switch an LED (p13) |
| Distortion LEDs | 2 LEDs (not bargraph). **The manual contradicts itself**: p1 "a 1% LED and a 'Redline' (3%) LED"; p5 "a yellow LED indicates .25% THD and the red 'redline' LED indicates 3% THD or more"; recall sheet legend "REDLINE" and "1% THD". Not reconcilable from the text |
| Output | "Locked, calibrated, output level"; **no unity-gain knob position is given** |
| Output level suggestions | output knob 8 for +4 tape level; 6.5 for -10 equipment; 6 to 7 for ADAT (p2, p9). I.e. knob 8 is about +4 dBu out when the input is driven until compression occurs (the manual does not give the input level) |

## 2. Ratios and curves (pages 1, 2, 4)

"Each ratio mode sets both the threshold and the ratio" (p4). The threshold in dBu is not given.

| Ratio | What the manual says |
|---|---|
| 1:1 | no compression, but the signal still passes "the warming circuits"; with the **British mode** switch it becomes the 1176 "all buttons in" curve (see 5) |
| 2:1 | "parabolic" knee, very gentle, "won't typically go into hard limiting" (so no overload protection). **Knee "+15 dB ... where the ratio gradually increases" (p2); "The 2:1 ratio has a knee that can be as long as 30 dB, depending on attack and decay settings" (p4)**. Special detector circuitry. "Silky smooth" |
| 3:1 | parabolic knee, gentle, no hard limiting; "very long knees of 2:1, 3:1, 4:1" (p2) |
| 4:1 | steeper knee, "gradually move towards hard limiting, 'nailing' the signal" |
| 6:1 | steeper knee, easy slope at first until after the knee, "where an increasing ratio 'musically' limits the peaks". General purpose; the recommended start (all knobs 5, 6:1: "you won't go wrong", p1) |
| 10:1 "Opto" | **shorter knee**; uses **separate detector circuitry** to emulate light-controlled devices (LA-2A, -3A, -4A). Recipe: attack 10, release 0, detector HP on; "keep the attack above 4 to keep the OPTO flavor; faster attacks give a more aggressive sound" (p4). Release goes up to **20 s** in this mode (p1 spec) |
| 20:1 | brick-wall-like limiting, "keeping any normal signal within 1 dB or so"; used like Nuke; hard knee |
| Nuke | "medium threshold but when the signal hits it, a nuclear blast won't budge the output level"; brick wall, "within 1 dB or so"; **release curve is logarithmic: "lets off quickly at first and then slows. This release curve is a big part of the Distressor's sound"**; can release very fast without much crackle, even on bass; special detector circuitry; the output level is "a little lower than the other ratios" in Nuke (p3). Room-mic recipe: [10, 6, 2.5, 6] or attack 4 / release 2 or attack 5 / release 3; 15 to 20 dB of compression "starting to sound about right" |

Knee statements (p4): "All curves with the exception of 20:1 and Nuke have dominant knees." "The 6:1 and 10:1 Opto ratios employ shorter knee limiting, reminiscent of some old classics from the 60's and 70's." "Most exceptional are the 2:1, 10:1 and Nuke ratios, which employ special detector circuitry." Each curve also has "its own release shape" and "the knees falling in slightly different places" (p1, p4). **No other knee width is given; 3:1/4:1/6:1/10:1 are only described as longer/shorter.**

Soft knee definition (p4): "the first few dB of gain reduction occur at very low ratios, gradually increasing as the signal increases ... The knee usually extends for a few dB and gradually flattens out toward a final ratio."

"The harder you drive, the more knee you'll hit, and the greater the ratio will be" (p2). A 2:1 stays gentle; "Only 1 LED should be lit" at 6:1 in normal use.

Warning (p2): cycling through 1:1 while in use "compression will be turned off and the signal will swell to its peak input levels".

## 3. Time constants (pages 1, 3, 11)

| Quantity | Value |
|---|---|
| Attack | **50 us to 30 ms** (spec p1; text p3 "variable from 50uS to 30mS"). The attack knob sets the whole range with one control |
| Release | spec p1: **0.05 s to 3.5 s**; text p3: "50mS to 3 seconds". (3 versus 3.5 s: a small contradiction in the manual) |
| Release, 10:1 Opto | up to **20 s** (spec p1) |
| Release shape | per ratio, "its own release shape"; Nuke logarithmic (fast first, then slow) |
| "Fast attack warning" | attacks near 0 can cause low-frequency clipping/crackle and pumping at 20 dB or more of reduction; with the detector HP on, "turning up attack (to 5 or above) will often eliminate the effect" (p10) |
| Knob positions that appear in recipes | attack and release numbers are on the 0 to 10 scale, e.g. 1176LN: 6:1, attack 0 to 3.5, release 1 to 10.5 ("a familiar sound is 6:1, att 2, rel 4"); Fairchild IGFET: 6:1, att 3 to 5, rel 2 to 7 (start 4 and 4); "Over Easy": 2:1 to 6:1, attack 9, release 2, clean; LA-2A: 10:1 Opto, att 10, rel 0 (p4); vocal: 6:1, attack 5, release 4, 3 to 17 dB of compression; bass: 4:1 or 6:1, attack 5, release 4; acoustic [7, 2, 5, 7] = input 7, attack 2, release 5, output 7; snare/kick/tom [3:1 to 6:1, 6, 5, 5, 6]. The **taper (knob number to ms) is not given**; only the end points |

## 4. Detector (sidechain) functions (pages 2, 6, 9; block diagram p6)

- The detector is the part "that figures out how much and how fast to turn down a signal. The actual audio you finally hear is not passed through this circuitry" (p6). Audio path: Input gain, differential amp, VCA, distortion generator, 80 Hz HP (in/out), output gain, output amp, relay bypass (block diagram, p6). Detector path: link sum, band emphasis (in/out), HP filter (in/out), detector circuit and envelope generator, control voltage back to the VCA, to the bargraph, and DC link (p6). The manual does **not** say whether the detector is feed-forward or feed-back.
- **Detector HP**: "a high-pass filter in the detector ... will not allow low, low frequencies to trigger compression" (p2). **The frequency and slope are not given.** Purpose: p/b blasts, low "sum and difference" frequencies pumping the highs, tom-induced modulation.
- **Detector Band Emphasis**: "an Eq inserted into the detector circuitry that makes the circuit much more sensitive to harsh, mid band frequencies" (p2); "emphasized **6 kHz** band makes unit overreact to harsh mids" (p6). **Boost amount and Q are not given.** Use: vocals with a nasty edge when they go up, guitars, synths, brittle piano notes; for solos, "pull up the lower, softer notes and push back the higher, thinner notes" (p3).
- **Stereo Link** (red LED): original link "uses a summing and phase detection method which allowed stereo image shifting" (p13). Two TRS cables, Link Out to Link In both ways, "Link" LED on both units. "Link sums 2 inputs" (p9). With the cables unplugged, "the distortion generator will receive a hotter signal since the detector is seeing half of what it would normally see in true stereo operation" (p9 trick 2): so link sums the two detector signals (and halves each when only one is present).
- **Image Link option** (EL8-X; p13): replaces image-shifting phase link; three link modes: the original "phase" link, the new Image Link, or both. With Image Link "the units will now always match in their gain reduction amount - eliminating image shift", the gain reduction of the two channels is locked; "the units can respond more to the louder channel if the input levels are not matched". Needs TRS plugs (the ring carries the image link, the tip the old link); more than two units can be daisy-chained, last output back to first input; avoid long cables. Master/slave trick: put the lower unit in 1:1 with attack and release on 10, the upper unit sets ratio and timing, so "the top unit must drive both units' timing circuitry" and gives a longer attack than otherwise possible ("a la SSL type compression"). Unmatched settings: "Usually the lowest attack/decay settings will override the higher settings" (p13). Metering mismatch of 1/10 dB between units is "expected".
- External sidechain trick (p9): Link Out to an EQ/preamp and back to Link In, with Link on; the sidechain "must not have appreciable delay nor be out of phase" because the link signal is mixed with the normal detector signal (delay causes combing).
- Detector input level scaling with ratio is not given.

## 5. Audio path modes (pages 5, 6)

| Mode | Manual statement |
|---|---|
| **Audio HP** | "rolls off low 'sub' frequencies below 80 Hz in the audio. It is a very smooth **Bessel filter, about 3 dB down at 65 Hz and -12 at 30 Hz**. Its final slope is **18 dB per octave**" (p5). Block diagram: "80 Hz High Pass Filter". (My check: a 3rd-order Bessel high-pass with -3 dB at 65 Hz is -1.9 dB at 80 Hz, -5.3 dB at 50 Hz, -13.6 dB at 30 Hz, so -12 dB is a rounded figure) |
| **Normal (Clean)** | "No induced distortion. THD hovering between **.025 and .3%**" (p5); low distortion, full frequency range, 2 Hz to 160 kHz (+0, -3 dB) in clean mode (spec p1) |
| **Dist 2** (yellow) | "Emphasized 2nd harmonic distortion", **THD hovering between .05 and 3%**; "Class A type warmth, producing mostly 2nd harmonic when compressing"; "especially while compressing" (p5). Tube-like (p2, p5) |
| **Dist 3** (red) | "3rd harmonic emphasized", **THD between .1% and 20%**; "adds 3rd along with 2nd harmonic" (p2); "basically caused by nonlinear gain that results with the top and the bottom of waveforms being flattened out. Analog tape saturates in this manner. The 3rd harmonic is induced in the Distressor **by increasing VCA output level**" (p5). "gradually flattens out the top and bottom of the waveform", "can look and sound very similar to tape distortion" (p2) |
| Frequency response in Dist modes | "Response is shaped in distortion modes (Dist 2, Dist 3)" (spec p1); the shaping is not described |
| Harmonic limit | harmonics well above 2nd/3rd "should be lower in amplitude (<-60 dB)" (p5) |
| Hard clipping | "No hard clipping until a few dB past 'Redline'" (p1). The distortion indicators come on "most frequently in Dist 3 mode" (p5) |
| Tape-like recipe | "2:1 mode with Dist 3 engaged, and compress 1 to 3 dB ... Tape goes in and out of saturation quickly, so fast attacks and decays are appropriate." Over-saturated tape: higher ratio, 1 to 5 dB. "More than 3 to 5 dB of reduction will sound less like tape, more like compression"; "with the quick release, 2nd harmonic will still be strong in Dist 3" (p2) |
| Where distortion shows | "more obvious on overall mixes and complex programs. On individual instruments, sometimes 3% distortion sounds 'fat' and 'analog'" (p5) |
| Long attacks and Dist | "Long attack times can clip transients in lower ratios when Dist 2 or 3 are employed" (p10) |
| Transformerless | "low transient intermodulation distortion" (p4) |

## 6. British mode (1176 "all buttons in", option; p12)

- A separate switch; works with any ratio, and the 1:1 position "becomes" the British ratio when the switch is on ("Put the unit in the 1:1 ratio and turn on the British Mode Switch ... that enables it").
- "had some elements of the unit's 20:1 ratio, but with an unusual knee and new envelope shape". To stay 1176-like, **attack "well under 4" (below 3 or 4)**; above that "you also incur a rise in some grunge (distortion) and see the THD indicator lights come on a lot more".
- "The unit will be slightly less colored when not compressing."
- Curves "can vary by +/- .5 dB from unit to unit" (the standard 7 ratios are uniform).
- Use as a peak skimmer; with Nuke + British, attack 1 to 2, release 0 to 5, 10 to 20 dB: saturates bass notes like a bass amp (p9). Reverse effect: Brit + Nuke, attack 2, release ~6.5, 20 to 30 dB (p12).
- "The original 7 ratios shipped with all Distressors" (p12): 2:1, 3:1, 4:1, 6:1, 10:1 Opto, 20:1, Nuke. 1:1 is the eighth position; British mode is an add-on.

## 7. Specifications (page 1 and elsewhere)

| Spec | Value |
|---|---|
| Frequency response | 2 Hz to 160 kHz (+0, -3 dB) in clean mode; shaped in Dist 2/3 |
| Dynamic range | 110 dB from max output to min output in 1:1 mode; "greater than 100 dB signal to noise in distort 3 mode" |
| Distortion | 0.02% to 20% depending on distortion mode and release time (clean .025 to .3%, Dist 2 .05 to 3%, Dist 3 .1 to 20%) |
| Time constants | attack 50 us to 30 ms; release 0.05 to 3.5 s; 10:1 Opto release up to 20 s |
| Coupling | DC-coupled output; "high quality input caps" |
| I/O | XLR fully balanced, transformerless, pin 2 hot (changeable to pin 3); also 1/4 in jacks; XLR output has separate amplifiers per pin; the output jack tip is wired to pin 2 |
| Power | typical 10 W, max 14 W; 115/230 V switch; fuse 0.5 A (0.3 to 0.6 A safe) |
| Noise example | a 90 dB S/N room mic with 20 dB gain reduction: noise floor raised to 70 dB (p10) |
| Memory | gold capacitor holds the front-panel settings for at least four weeks (p11) |
| Distortion LEDs | see section 1 (0.25% or 1% for the first; 3% for the red LED) |
| Size | single height (1.75 in), 12 lb shipping weight |
| Components | metal film and Roederstein resistors in the audio path; hand-wired, calibrated in the USA |

## 8. Usage recipes (useful as sanity targets for a model)

- Vocals: compress 3 to 17 dB; "in soft passages no compression should occur while on loud passages 17 to 20 dB" (6:1, attack 2.5 to 3.5, release 0 to 2, HP and Dist 2).
- "Hit the unit quite hard (10 to 20 dB of compression on peaks)" in Opto mode.
- Room mics: Nuke or 20:1, 15 to 20 dB.
- Drums: "keep the attack over 3 to keep transients"; kicks with Opto (10:1, attack 10, release 0) and Det HP on.
- Parallel use ("Multing"/"Splitting") on snare: compressed signal mixed with dry, very quick release brings up ring and body.
- Distressor knobs "5 5 5 5" at 6:1 is the manual's neutral start.

## 9. What the manual does NOT give (do not assume)

- Threshold level of any ratio (dBu), gain range of the Input and Output knobs (dB), unity position of Output.
- Knee widths other than the 2:1 +15 dB (up to 30 dB) statement.
- Detector HP corner and slope; the Band Emphasis centre (a "6 kHz band" is named) boost and Q.
- Attack and release times at intermediate knob positions; the release time per ratio.
- Frequency response in Dist 2 and Dist 3; harmonic balance in dB at a given level.
- Any statement of detector type (peak/RMS), feed-forward or feedback.
- The 1:1 "warming" behaviour in numbers beyond the 0.025 to 0.3% THD figure.

## 10. Illegible / cut-off items

- Eighth detector state (list stops at seven).
- The "Distortion Settings" and "Advanced Detector functions" paragraphs on page 2 are complete; page 6 has a few fragments from the neighbouring column of page 1 mixed into the left margin.
- No images: the block diagram on page 6 comes through as text only (labels, no values).
