# Fender schematic readings vs what the code and docs assume

Scope: the Fender readings in this folder (Twin AB763/AA270/'65, Deluxe Reverb AB763/AB868/AA1172/AA1270/'65/'68 Custom, Super Reverb AB763, Princeton Reverb AA1164/AB1270, Vibrolux Reverb AB568/AA270, Vibroverb 6G16/AA763/AB763/'63/'64, Reverb 6G15, Bassman 5F6-A/6G6-B/AB165, Bandmaster 5E7, Deluxe 5A3/5B3/5C3/5E3/Model 26/AB763, Tweed Twin 5F8-A, Tweed Super 5E4-A/5F4, Princeton 5F2-A, Champ 5F1, Vibro-Champ 0082002000, Hot Rod DeVille, plus the JTM45 tremolo-chassis sheet as the British cousin of the 5F6-A) against the amp code and docs of StarterAmp_v0.1.

Nothing in the code or the docs was edited. Nothing was run or listened to; the code was read, not executed. The audible effects below are directions only, derived from the circuit, not measurements.

How to read it:
- "Code" = Source/PluginProcessor.cpp (voicing rows, `applyComponentVoicing`, `applyRealism`), Source/PowerSection.h (`supplyFor`, `familyForModel`, `loopGainFor`), Source/StageShaper.h (`planFor`, `arch`, `defPwr`), Source/ToneStackTmb.h, Source/FxExtra.h, Source/StageCurves.h comments. "Docs" = CIRCUITS.md, MODELED_GEAR.md, AMP_SOURCES.md, BASED_ON.md, TUBES.md, TRANSFORMERS.md.
- "Sheet" = the reading file named in the cell. "(?)" = uncertain in the reading or in my interpretation. "(computed here)" = my arithmetic from printed values. Sheet voltages are printed with +/-10 or 20 % tolerance, so differences under about 20 V are not meaningful.
- Confidence: H = both sides are plainly stated and I checked them; M = one side is inferred or the sheet is ambiguous; L = guess or both sides weak.
- The sag figure used below is `10 x Ipk x Rs / B+` (the code's depth before the Sag knob, from `kSupplyCal*5*Ipk*Rs/B+*2*sagAmt`). The feedback "dB" values are the divider fraction beta only, not loop gain.

---

## 0. The ten things that matter most

1. Twin Sparkle: the docs cite AA270 (cold silverface) values for the PI and droppers (2.2k and 10k droppers, 330k grid leaks, 47k loads, 2 nF), but the code's supply is 480 V, above all three Twin sheets (AB763 +458/460, '65 +442, AA270 +405). So the amp is a mixture, and its B+ is the stiffest of the lot. (H)
2. Twin Sparkle's preamp stage curve is the "low-gain blackface" arch 0, which is solved at plate +170 V, cathode +1.2 V (StageCurves.h header, "B=250, Rp=100k, Rk=1500"). That is the Deluxe Reverb operating point (plates +170/+180, cathode +1.3 V), not the Twin's (AB763 +240 to +270 V, cathode +2.0 V; AA270 +210 V, +1.6 V). (M: arch-to-header mapping inferred)
3. Cedar Spring is documented as a Deluxe-Reverb-class amp (2 x 6V6, about 22 W, B+ 420 V fits the DR's +415/+420 V), but it takes the Twin's feedback family (820 ohm into 100 ohm, beta about -19 dB) where every Deluxe Reverb sheet has 820 ohm into 47 ohm (beta about -25 dB). The code has about 6 dB more feedback than a DR, so it is tighter and less loose. (H on numbers, M on audible size)
4. Cedar Spring is documented with a silicon rectifier; every Deluxe Reverb sheet has a tube rectifier (GZ34 on AB763, 5U4GB on AB868/AA1172/AA1270, 5AR4 on the reissue and '68 Custom). (H)
5. Tweedle Dee's supply time constant 0.075 s equals 4.7k x 16 uF; the 5E3 sheet shows 5000 ohm (0.080 s). Fine. But the docs' "470 ohm / 1 W screens" and "345 V / 320 V / 224 V" are not on the 5E3 sheet: the sheet shows screens straight to the 5000 ohm node and prints no voltages. (H)
6. Tweedle Dee "Four-ten" (a Bassman idea) inherits the 5E3's 335 V supply, 6V6 power curve, no feedback, single tweed tone control and cathodyne-style tone. A 5F6-A Bassman is a push-pull 5881/6L6 amp at +432 V with a GZ34, a 27k feedback loop with a 5k presence control, a cathode-follower Bass/Middle/Treble stack and a long-tail PI. Large difference on every item. (H)
7. The 6V6 power curve (`pwr_2`) is listed in StageCurves.h with bias -15 V and idle 150.7 mA per tube. The Fender 6V6 sheets give 24 to 39 mA per tube (Vibro-Champ +18.2 V across 470 ohm, Champ 5F1 +18 V across 470 ohm, DR '68 Custom "24 mA idle") and fixed bias of -34/-35 V (Princeton Reverb, DR). Either the table comment is mislabelled or the curve sits in a very different operating region. I could not verify. (M)
8. The Sparkle spring return is faithful on the high-pass (0.003 uF into the 100k pot = 530 Hz, computed here, on Twin AB763/AA270 and every DR/Vibroverb/Princeton blackface sheet; the '65 Twin and '68 Custom use 0.0033 uF = 482 Hz). It lacks the real amp's reverb-send high-pass (500 pF into 1M = 318 Hz, computed here) and the fact that the real return enters before the phase inverter and is shaped by the power stage. (H on values, M on audible size)
9. Gaps with a schematic in hand: single-ended Champ/Princeton (5F1, 5F2-A, Vibro-Champ), Princeton Reverb (2 x 6V6, no choke), brownface Vibroverb 6G16 (weak 10k feedback, bias tremolo), Super Reverb (GZ34, 2 x 6L6), Bandmaster 5E7, cold silverface Twin (AA270), tweed Bassman 5F6-A. None needs a new engine except the single-ended amps.
10. The AA270 sheet does print voltages (+405 V plates and screens, +380 V C, +315 V D, preamp plates +210 V), but CIRCUITS.md says of the Twin "No voltages are printed". Either the docs read a different sheet or the sentence is out of date. (M)

---

## 1. Model by model: what the code/docs assume vs what the schematics say

### 1.1 Twin Sparkle (model 0, "1972 Twin")

Revision: docs say "1972 Twin" (BASED_ON.md, AMP_SOURCES.md). The values quoted in CIRCUITS.md (2.2k then 10k droppers, 330k grid leaks, 68k grid returns, 47k plate loads, 2 nF per phase) are the AA270 values (TwinReverb_AA270.md, dwg 045377 Rev E; the revision block date reads "1-30-70 (?)"). The AB763 (blackface) and '65 Reissue values differ. See `TwinReverb_revisions.md`.

| Item | What the code/docs assume | What the schematics say | Difference | Likely audible effect (direction only) | Conf. |
|---|---|---|---|---|---|
| HV winding / B+ at plates | `supplyFor(0)` = 480 V, Ipk 0.45 A, Rs 60 ohm, tauSlow 0.044 s, otSat 0.15. Sag figure 10 x Ipk x Rs / B+ = 0.56 | AB763: 340 V AC per side, plates +460 V (rail +460). AA270: 315 V AC, plates and screens +405 V (rail printed +410 "STANDBY ON" / +450 "STANDBY OFF", meaning unresolved). '65 RI: bridge, +442 V (TP27) | Code 480 V is 20 V above AB763, 38 V above '65, 75 V above AA270 | Cleaner for longer, later power-stage clipping, less sag depth than the cited AA270; closest to AB763 | H (numbers), M (what 480 means: loaded or no-load) |
| Rectifier | Silicon (Rs 60 ohm), tau 0.044 s | AB763 and AA270: 6 diodes, 3 per leg; '65: bridge CR1-CR4 | Consistent | None | H |
| Reservoir | tauSlow 0.044 s = 2.2k x 20 uF (the B-to-C dropper). Reservoir itself not modelled | AB763/AA270: 2 x 70 uF/350 V series = 35 uF (computed). '65: 2 x 220 uF/285 V series = 110 uF (computed). Dropper B-to-C: AB763 1k (0.02 s with 20 uF), AA270 2.2k, '65 1k | tauSlow follows AA270's 2.2k; the B+ and the AB763-like 460 V rail do not | Only matters for the sag recovery speed: AB763/'65 would recover faster | M |
| Preamp B+ chain | Preamp rail 10k x 20 uF = 0.2 s (CIRCUITS.md) | AA270: 10k to D (+315 V (?)). AB763: 4.7k to D (+410 V). '65: 4.7k R68 (+393 V). | Code/docs use the AA270 10k but the AB763-like voltage | If AA270: preamp B+ about 95 V lower, so the preamp clips earlier; code is clean/stiffer | H (values), M (effect) |
| Preamp stage operating point | `planFor` arch {0,0,0} = "low-gain blackface", curve header "B=250, Rp=100k, Rk=1500, vk=1.20, vp0=170 V, gain 60.6" | AB763 plates +260/+270/+250/+240 V, cathode +2.0 V. AA270 plates +210 V, cathode +1.6 V. '65: supply X +393 V, cathodes +1.9 to +2.0 V | Curve sits at the Deluxe Reverb point (170 V, 1.2 V), 40 to 100 V lower in plate voltage than the Twin | Preamp stages clip earlier and softer than a Twin's; headroom of the real amp's front end is larger, especially on AB763 | M |
| Cathodes | `applyComponentVoicing(0)`: first stage 1.5k / 25 uF; second coupling 0.02 uF into 1M | V1A and V2A 1.5k // 25 uF; V1B/V2B/mixer shared 820 // 25 uF. Normal channel V1A plate goes directly to the stack; V1B output is 0.047 uF into 220k | Second stage uses 1.5k/25 uF in code; sheet has 820 ohm there. The 0.02 uF/1M (8 Hz) in code vs 0.047 uF/220k (15 Hz) on the sheet | Both corners are sub-bass; negligible | H |
| Tone stack | ToneStackTmb type 0: 250k treble, 250k bass, 10k mid, 100k slope, 250 pF, 0.1 uF, 0.047 uF, 38k source, 1M load | Same on AB763, AA270 and '65: 100k, 250 pF, 0.1 uF, 0.047 uF, 250k/250k/10k pots | None | None | H |
| Bright cap | 120 pF on a 1M volume | 120 pF with switch on the Twin volume | None | None | H |
| Phase inverter | Not modelled (docs: "Not applied: the phase inverter"). Docs list "330k, 33k, 470 ohm, 47k loads, 68k to the grids" | AA270: .01 uF in, 330k leaks, tail 270 ohm + 22k + 100 ohm (about 22.4k), 47k/47k loads, 68k returns, 2000 pF per phase. AB763 and '65: .001 uF, 1M leaks, tail 470 ohm + 22k + 100 ohm, 82k/100k loads, 220k returns, no 2000 pF | Docs mix AA270 (330k, 47k, 68k) with AB763 (470 ohm tail) and write "33k" where the sheet reads 22k | Not in the signal path today, so nothing is audible. If added: AA270 passes more bass (48 Hz vs 159 Hz corner, computed) and is darker (rough 9 kHz pole, computed, assumption-heavy) | H (values), L (the 9 kHz) |
| Screens | Not explicit | 470 ohm 1 W from B+, all three sheets | None | None | H |
| Bias | 6L6 power curve (`defPwr[0]` = 1): `pwr_1_med` bias -53 V, idle 46.3 mA/tube; `pwr_1_hot` -46 V, 92 mA | AB763 -52 V fixed. AA270 raw -52 V, wiper -44 V (topology unresolved). '65 -50 V at the wiper, range about -43.5 to -59.7 V (computed) | "med" matches AB763; AA270 and '65 sit a few volts hotter | AA270/'65: slightly warmer idle, less crossover | M |
| Feedback | Family 0: 820 ohm into 100 ohm, about -19 dB; loop gain 1.35, 2.83, 4.68 | AB763, AA270: 820 ohm into a node with 22k and 100 ohm: beta = 100/920 = 0.109 (-19.3 dB, computed here). '65: same plus 1500 pF across the 820 ohm (corner about 129 kHz, computed) | None in the audio band | None | H |
| Presence | None | None on any of the three | None | None | H |
| Output transformer | `otSat` 0.15; Hammond 1750KA data in TRANSFORMERS.md is not used by the model | AA270 prints "TOTAL LOAD 4 ohm", OT TR3-125A29A; primary impedance, saturation and leakage not on any sheet | No schematic data for saturation | Unknown | L |
| Reverb return (Sparkle) | See 1.6 | See 1.6 | See 1.6 | See 1.6 | |
| Vibrato / tremolo | Stand-alone FX with Optical and Bias shapes; generic rate and depth | Opto (neon + photocell). Oscillator V5 12AX7, SPEED 3M-RA + 100k, INTENSITY 50k-RA fed from the mixer plate node N2 into a photocell shunt; Vibrato channel only; 2.2M ties the network to the raw bias node | Real tremolo is on the vibrato channel only and sits in the mixer stage, before the PI | The plugin tremolo is simply amplitude modulation on the whole path: close in character for opto, but all paths get it | M |

### 1.2 Tweedle Dee, Soft and Cranked (model 9 voicings 0 and 1; docs: 5E3)

Revision: Deluxe_5E3.md. Sheet has a schematic and a layout, no printed voltages, no date or revision. Sheet neighbours with voltages: 5B3 (plate/CT +350 V, screens +280 V, preamp +250 V, 10k/10k; cathode +18 V), Champ 5F1 (+340/+295/+250 V).

| Item | What the code/docs assume | What the schematics say | Difference | Likely audible effect (direction only) | Conf. |
|---|---|---|---|---|---|
| Rectifier | 5Y3, Rs 250 ohm (docs: estimate) | 5Y3GT | None | None | H |
| Supply B+ | `supplyFor(9)` = 335 V, Ipk 0.12 A, Rs 250, tauSlow 0.075 s, otSat 0.40. Docs: "345 V plates, 320 V screens through 4.7k x 16 uF, 224 V preamp through 22k". Sag figure 0.90 | 5E3: no voltages printed. Chain 5000 ohm to screens, then 22k to preamp, all 16 uF/450 V. 5B3 +350/+280/+250; 5F1 +340/+295/+250 | 335 V vs docs 345 V (docs and code disagree by 10 V); both match the family; 4.7k vs 5000 ohm (6 %); the 320 V/224 V are the docs' own numbers | Within tolerance: sag character about right; the 5E3 B+ cannot be confirmed from this sheet | M |
| Screen resistor | Docs (MODELED_GEAR.md line 180): "470 ohm / 1 W screens" | Screens direct to the 5000 ohm node, no screen resistor | Docs add a resistor the sheet does not have | If the code includes it: slightly less screen current draw sag and a slightly stiffer screen in code than the amp; flatter compression. Unknown whether the code uses it | M |
| Bias | Cathode bias: 250 ohm 5 W // 25 uF shared (docs). Power curve `pwr_2` (6V6): med bias -15 V, idle 150.7 mA/tube; hot -12 V, 176.9 mA | 5E3: 250 ohm 5 W // 25 uF shared; no voltage printed. 5B3 prints +18 V cathode; with 250 ohm that is about 72 mA both tubes, about 36 mA per tube (computed here, assuming the 5E3 sits near the 5B3 point) | Table comment 150.7 mA/tube is about 4 times higher than any Fender 6V6 sheet gives (24 to 39 mA per tube) | If the curve's idle really is 150 mA, the tube behaves as heavily class A: smoother, more even-harmonic, less crossover | M (curve provenance not verified) |
| Feedback | Family 3 = none | No NFB on the 5E3 (the 5C3 had it, removed on the 5E3) | None | None | H |
| Preamp tubes | 12AY7 stage 1 estimated at 25k plate resistance | 12AY7 (both V1 halves), 12AX7 V2 | None | None | H |
| Cathodes | `applyComponentVoicing(9)`: 820 ohm / 25 uF, 1.5k / 25 uF, 0.1 uF into 220k; all clamped to 25 Hz or higher | V1a/V1b 820 // 25 uF (shared), V2a 1.5k // 25 uF, 0.1 uF to the 1M volumes, 0.02 uF to the PI | Code coupling uses 220k (sheet has 1M pots); clamp raises sub-8 Hz corners to 25 Hz | Slightly less very-low bass in code than a real 5E3 | M |
| Stage curves | `planFor` arch {1,1,1} = "cold class-A", curve header "B=220, Rk=2200, vk 1.26, vp0 163 V" | 5E3 V1 cathode 820 ohm (12AY7), V2a 1.5k; 5B3 preamp node +250 V | Curve cathode resistor 2.2k vs 820/1.5k | Stages in code run colder than the real 820 ohm stage: less gain and an earlier-onset softer clip in V1 than the real amp | L |
| Tone | `TweedTone`: 500 pF top, 4.7 nF bottom, 1M pot, driven by the Treble knob. Docs: "exactly solved" | One 1M tone pot, 0.0005 uF to the pot top, .005 uF at the bottom (layout shows ".005 600"), wiper to the channel-1 mix node; tone acts on channel 1 only | 4.7 nF vs 5 nF (6 %); channel 2 (the other V1 half) has no tone control and is not modelled | Negligible; the "bright" second channel is absent in the plugin | H |
| PI | Cathodyne; docs: "Not modelled: the cathodyne inverter's imbalance" | V2b cathodyne, 56k plate load, 1.5k + 56k cathode | None | Plugin is a touch more symmetrical than the real amp | H |
| Output stage | Grid stoppers 1.5k, 220k leaks | 1500 ohm stoppers, 220k leaks | None | None | H |

### 1.3 Tweedle Dee, Four-ten voicing (model 9 voicing "Four-ten")

Docs: "A 4x10 tweed Fender Bassman-style voicing", a hand-tuned row (PluginProcessor.cpp around line 550). It still runs on model 9's supply, power curve (6V6), tone network (TweedTone) and no-feedback family, so every circuit item below is the 5E3's, not a Bassman's. Reference sheets: Bassman_5F6A.md (also Bassman_6G6B.md, Bassman_AB165.md; JTM45_tremChassis.md for the British cousin).

| Item | Code (Four-ten uses the 5E3 settings) | 5F6-A (tweed Bassman) | Difference | Likely audible effect | Conf. |
|---|---|---|---|---|---|
| Output tubes | 6V6 curve (`defPwr[9]` = 2) | 2 x 5881, fixed bias | Different tube family, class AB with fixed bias | Real amp is stiffer with a lot more headroom and bass tightness | H |
| Rectifier / B+ | 5Y3, 335 V, Rs 250, tauSlow 0.075 s | GZ34; 325 V AC per side; +432 V at plates, +430 V screens, 4700 ohm to +385 V (PI), 10k to +325 V (preamp) | About 100 V higher B+ and a lower-resistance rectifier | Real amp should sag less (stiffer tube-rectifier supply with a choke and 40 uF-class reservoirs; the sheet gives no Rs, so the amount is unknown) | H (B+), L (amount) |
| Feedback | None (family 3) | 27k from the OT secondary to the PI circuit, 5k PRESENCE with 0.1 uF, 10k to the PI junction (the fraction is not computed in the reading) | Loop missing in code | Real amp is tighter, less fizzy, with a presence control; the code is looser and rougher at the bass | H |
| Tone | TweedTone single tone control | Cathode-follower-driven stack: 56k, 250 pF, three 0.02 uF caps, 250k treble, 1M bass, 25k middle | Topology and controls differ | The scooped-mid Bassman response and the interactive Bass/Mid are not there | H |
| Preamp | Same V1 12AY7 / V2a | V1 12AY7 shared 820 ohm // 250 uF, V2a 12AX7 mixer with unbypassed 820 ohm, V2b cathode follower (DC-coupled), PI long-tail 12AX7 (82k/100k, 47 pF plate to plate) | Several extra stages and a long-tail PI | Real preamp is cleaner/brighter before the stack | H |
| Bias | 6V6 cathode-bias curve | -48 V fixed, 15k + 56k from the bias tap, no pot | Different | Not easily audible | H |
| Screens | not modelled separately | 470 ohm 1 W to +430 V | Different from 5E3 (screens direct) | Slightly softer screen drive | H |
| Fuse/OT | n/a | OT 45249, 4 jacks for 4 x 10, impedance not printed | None possible | None | |

### 1.4 Cedar Spring: Blackface, Spanky, Cranked (model 30 voicings 0, 1, 2)

Docs: "small blackface-style combo, two 6V6, about 22 W, silicon rectifier, small transformer... Supply numbers (B+ 420 V, peak current and source resistance) are estimates, not read from a schematic... No schematics yet" (MODELED_GEAR.md 221, BASED_ON.md "Own design; Twin-based estimates"). The Blackface voicing gain structure is fitted to twelve Deluxe Reverb captures (kFitLayers, PluginProcessor.cpp ~1389). `supplyFor(30)` = {420 V, 0.20 A, 110 ohm, 0.12 s, 0.30}; sag figure 0.52; `familyForModel(30)` = 0 (the Twin's -19 dB family); `planFor` is inactive for models above 28, so Cedar uses the classic (non-circuit) stage shapes; `applyComponentVoicing` has no entry for 30. The nearest sheets are the Deluxe Reverb set (DeluxeReverb_revisions.md) and Princeton Reverb.

| Item | What the code/docs assume | What the schematics say | Difference | Likely audible effect | Conf. |
|---|---|---|---|---|---|
| Rectifier | Silicon (docs) | DR: GZ34 (AB763), 5U4GB (AB868, AA1172, AA1270), 5AR4 ('65, '68). Princeton Reverb: GZ34 (AA1164) or 5U4GB (AB1270). Deluxe Reverb never has diodes | Docs contradict the DR family | Real DR sags more (tube rectifier); code is stiffer unless Rs 110 offsets it | H |
| B+ / plates | 420 V | DR AB763: HV 330 V AC per side, plates +415, OT CT +420, screens +415, B +410. '65: +391 V. '68 Custom: +407 V. Princeton Reverb: 340 V AC, +420 CT, B +400 | Code matches AB763/AB868/SF DR | None | H |
| Reservoir / droppers | tauSlow 0.12 s | DR AB763/AB868: 2 x 16 uF parallel (32 uF), choke, 10k then 10k (to C, D). AA1172/AA1270: 2.2k then 10k. '65/'68: 10k, 10k, 22 uF caps. Princeton Reverb: no choke, 1k, 18k, 18k RC ladder with 20 uF caps | 0.12 s equals none of 10k x 16 uF (0.16 s), 2.2k x 16 uF (0.035 s) or the Princeton 18k x 20 uF (0.36 s) | Provenance of 0.12 s unknown | L |
| Sag figure | 10 x 0.20 x 110 / 420 = 0.52 (Twin 0.56) | No sheet gives rectifier or transformer resistance | Cedar sags about 7 % less than the Twin in code, despite 2 tubes vs 4 and a tube rectifier on the real DR | Real DR/Princeton family should compress/sag audibly more than a Twin; the code makes them nearly equal | L |
| Feedback | Family 0: 820 into 100 ohm = -19 dB | DR (all revisions): 820 ohm into 22k / 47 ohm node = 47/867 = 0.054 (-25.3 dB, computed in the DR revisions file). '68 Custom: 1.5k, 47/1547 = 3.0 % (-30.3 dB). Princeton Reverb: 2700 ohm into the V3A cathode junction (47 ohm): 47/2747 = 1.7 % (-35 dB, computed here) | Code has about 6 dB more feedback than DR AB763, 11 dB more than the '68 Custom, 16 dB more than Princeton Reverb | Code is tighter, lower output impedance, less speaker-interaction looseness and bloom | H |
| Tone stack | Twin-style stack: 10k Middle pot | DR: 250 pF, 100k, 0.1 uF, 0.047 uF, with a fixed 6.8k to ground in place of a Middle pot on every sheet; '68 Custom Custom channel: 0.022 uF and 18k + 6.8k (24.8k). Princeton Reverb: same 6.8k, same caps | A Middle knob that is not on the amp; DR mid is fixed at 6.8k | Mid depth is about right with the Middle knob near 6.8k/10k = 0.68 (taper unknown, so approximate) | M |
| Bright cap | Spanky is a hand-tuned row; no cap value | DR: 47 pF on the Vibrato channel volume only. Twin: 120 pF. '68 Custom: removed | A real DR "Spanky" is mild (47 pF) next to the Twin | Spanky as a DR would be less bright than the Twin's 120 pF | H |
| Preamp operating point | Classic shapes | DR plates +170 to +180 V, cathode +1.3 V (the same operating point as the arch 0 curve) | Right point, but not applied to model 30 | Unknown | M |
| Bias / tube | No circuit power curve for model 30 (classic shaping); power tube comes from the Power Tube menu | DR -35 V fixed ('65 -37 V, '68 -38 V with 24 mA set). Princeton Reverb -34 V | Bias is not represented as a number for this amp | n/a | M |
| Cranked | `pwrAdd` 0.30 | DR AB763: no master volume; "Cranked" is a volume-knob state, not a circuit | None | None | |

### 1.5 Cedar Spring: Big Spring and Quad Ten (voicings 3 and 4)

Docs: Big Spring "a 40 W two-6L6 combo with a 15-inch speaker" (gain structure fitted to Vibroverb amp-only captures); Quad Ten "a 60 W two-6L6 combo with four 10-inch speakers and a solid-state rectifier" fitted to Blues DeVille Direct Out captures. Both still use model 30's supply {420 V, 0.20 A, 110 ohm}, `familyForModel` 0, pwrAdd 0.05; no circuit stage curve (planFor inactive for model 30). The code comment on line 1051 says "two 6V6s, about 22 W; a 15-inch-cab variant as the 4th voicing" which conflicts with the docs' "two-6L6".

| Item | What the code/docs assume | What the schematics say | Difference | Likely audible effect | Conf. |
|---|---|---|---|---|---|
| Big Spring: reference amp | Vibroverb, 40 W, 2 x 6L6, 15" | Vibroverb AB763: 2 x 6L6GC, GZ34, plates +460 V, 355 V AC per side, bias -55 V, grid stoppers 1500 ohm, NFB 820 ohm and 47 ohm (-25 dB), PI 12AT7 (82k/100k, tail 22k/470/47). Vibroverb AA763: +460 V, 100k/100k PI loads. '63 RI: +485 V, bridge | Code 420 V is 40 V below the AB763 | Code is softer/earlier compressing than a real Vibroverb's 460 V 6L6 stage | H |
| Big Spring: power tube | No circuit power curve (classic shaping); the docs and code comment disagree on 6L6 vs 6V6 | 2 x 6L6GC | Tube class depends on the Power Tube menu choice | If a 6V6 is selected: saturates earlier and rounder; the real stage has more headroom | M |
| Big Spring: feedback | -19 dB | -25 dB | Code has 6 dB more | Tighter | H |
| Big Spring: rectifier | Silicon | GZ34 | Different | Less sag | H |
| Quad Ten: reference amp | Blues DeVille 410 (60 W 2 x 6L6, 4 x 10, solid state). Only a Hot Rod DeVille sheet (not a Blues DeVille) is in the folder | Hot Rod DeVille: 2 x 6L6GC, solid-state bridge, plates/screens +483 V, 470 ohm screens, bias -54.6 V adjustable, NFB 47k into 4.7k (about -21 dB, computed in DR revisions file) plus presence R67 25k + 0.1 uF, droppers 4.7k and 10k, 3 x 12AX7A, relay-switched Drive channel | Code 420 V vs 483 V; code has no presence control and a Twin-type -19 dB; solid-state is consistent | Real amp has about 63 V more B+ and more headroom/tighter lows. Feedback depth is close (-19 vs -21 dB) | M (the Blues DeVille is not the same schematic) |
| Quad Ten: power tube | As above | 2 x 6L6GC | As above | As above | M |

### 1.6 The Fender-style "Sparkle" spring return (FxExtra.h `SpringReturn::setStyle` case 1; CIRCUITS.md spring table)

Code: high-pass 530 Hz (0.003 uF into the 100k Reverb pot), a peak at 4 kHz Q 1.2 +4 dB standing for the pickup with 2 nF across it (an assumption, coil inductance is not on any sheet), no high-pass on the send, reverb after the cab by default (no in-power-amp injection).

| Item | Code/docs | Sheets | Difference | Likely audible effect | Conf. |
|---|---|---|---|---|---|
| Return coupling | 0.003 uF into 100k = 530 Hz (computed) | Twin AB763/AA270, DR AB763/AB868/AA1172/AA1270, Vibroverb AA763/AB763, Princeton Reverb AA1164/AB1270: 0.003 uF into the 100k-L pot. Twin '65 and DR '65/'68: 0.0033 uF = 482 Hz (computed). Vibroverb '63 RI .0033, '64 .0033 | Matches the blackface family; the reissues are 10 % lower | Reissues slightly fuller | H |
| Pickup loading | "2 nF + 220k across the pickup" (docs) | DR AA1172/AA1270 and Princeton AB1270: 220k // 2000 pF. Twin AA270: 220k in series with 0.002 uF to ground (?). AB763 sheets: 220k only. '68 Custom: 220 pF plus 10k stopper | Docs' wording fits the DR/Princeton silverface, not Twin AB763, and the Twin AA270 reading is series (?) | The "sizzle" peak only exists on silverface units; blackface has none | M |
| Send | No high-pass | Driver 12AT7 pair, grid 1M, input from the vibrato-channel plate through 500 pF: corner 1/(2 pi x 1M x 500 pF) = 318 Hz (computed here). CIRCUITS.md itself says "so the driver gets a bass-cut signal" | Code lacks this high-pass | Plugin reverb has more low-mid wash than the amp's; the real send is thin | H |
| Driver | Not modelled | 12AT7 paralleled, cathode 2.2k // 25 uF, transformer TR4 | Not modelled | Clean transformer-coupled drive that clips softly when driven hard | H |
| Return level / injection | Mixed after the cab | Wiper through 470k into the mixer grid with 220k to ground: attenuation 220/(470+220) = 0.32 (computed in the Twin reading). '68 Custom 1.5M instead of 470k (-8 dB, computed in the DR file). The return enters before the PI and is amplified, saturated and sagged by the power stage | Plugin reverb is outside the amp | Amp's reverb is compressed with the dry signal on loud notes; plugin's is clean and constant | M |
| Which channel | All input | Twin/DR/Super: reverb driver fed from the Vibrato channel only; the Normal channel has no reverb. '68 Custom: both channels | Plugin reverb is on everything | n/a | H |
| Tank | Not on any sheet; CIRCUITS.md quotes the Type-4 4AB3C1x tank as "the usual Fender part", not from the sheet | Not named on the Twin sheet | n/a | n/a | L |
| Outboard 6G15 | Not in the plugin | 6K6GT driver with 0.002 uF into 220k (362 Hz high-pass, computed in the reading), 1.5k bypassed cathode, 7025 recovery with 0.1 uF into a 50k TONE and 250 pF into the MIXER, DWELL 250k, solid-state half-wave rectifier, +295 V | A candidate for an eighth style | Drier, boomier-send, "tone"-adjustable spring | M |

### 1.7 Princeton / Champ-style small amp (not modelled)

The plugin has no single-ended amp and no Princeton or Champ. The nearest sound in the plugin is Tweedle Dee Soft ("a small, low-power, two-stage amp. Early, soft breakup") but that is a push-pull 5E3 design. Sheets available:

| Amp | Output stage | Supply | Feedback | Tone / extras | Notes |
|---|---|---|---|---|---|
| Champ 5F1 (Champ_5F1.md) | 1 x 6V6GT single-ended, cathode 470 ohm // 25 uF (+18 V), screen direct | 5Y3GT; +340 V OT CT, +295 V screens (10k), +250 V preamp (22k); 16/8/8 uF; plates +150 V | 22k from the OT secondary to V1b cathode (1500 ohm): beta 1500/23500 = 6.4 % (-23.9 dB, as stated in the reading) | No tone control; V1a cathode unbypassed; 0.02 uF/600 V couplings | Very simple; printed 38 mA total cathode current (18 V / 470, computed in the reading) |
| Princeton 5F2-A (Princeton_5F2A.md) | Same 6V6, cathode 470 ohm 1 W // 25 uF | 5Y3GT; two 16 uF on node 1; 10k 1 W; 8 uF nodes; 22k; 3/4 A fuse | 22k to V1b cathode, 6.4 % | Tone control 1M LIN (.005 / .0005), V1a bypassed | No voltages printed |
| Vibro-Champ 0082002000 (VibroChamp_0082002000.md) | 6V6GT single-ended; cathode 470 ohm 2 W // 25 uF (+18.2 V); screen via R11 470 ohm 2 W | 5Y3GT plus BYD33V diodes; 350 V / 305 V / 265 V; 22 uF, 10 uF, 10 uF; 10k, 22k | 2.2k into 47 ohm (R17) at V1-B cathode: 2.1 % (-33.6 dB) | Bias-style tremolo into V1-B cathode via 3.3k; speaker-level switch | Fully legible, all test points printed. Plate +340 V, AC 185 V at TP7 |
| Princeton Reverb AA1164/AB1270 | 2 x 6V6GTA, fixed bias -34 V, 220k leaks, no grid stoppers, cathodyne PI (56k, 1k + 56k) | 340 V AC; 5U4GB (AB1270) or GZ34 (AA1164); no choke; 1k to B +400 V, 18k, 18k RC ladder to +320/+240 V | 2700 ohm into the V3A cathode junction (47 ohm), 1.7 % (-35 dB, computed here) | Single channel, reverb, bias-modulation tremolo (INTENSITY 250k into the -34 V line), 6.8k fixed mid | Plates +410 V, screens +400 V |

### 1.8 Deluxe-style supply values (what the code could take from sheets)

| Amp | Rectifier | Plates / CT | Screens | Preamp | Reservoir | Droppers |
|---|---|---|---|---|---|---|
| DR AB763 | GZ34 | +415 V (CT +420) | +415 | B +410, C +325, plates +170 to +180 | 2 x 16 uF parallel, choke | 10k, 10k |
| DR AB868 | 5U4GB | same | same | same | same | same |
| DR AA1172 | 5U4GB | same | same | C +375 | same | 2.2k, 10k |
| DR AA1270 | 5U4GB | same | same | C unreadable | same | 2.2k, 10k |
| DR '65 | 5AR4 | +391 V | not printed | Y +311, X +261 | 38.7 uF (computed) | 10k, 10k |
| DR '68 Custom | 5AR4 | +407 V | not printed | Y +324, X +270 | 38.7 uF | 10k, 10k |
| Princeton Reverb AB1270 | 5U4GB | +410 V (CT +420) | +400 | +320, +240 | 20 uF caps, no choke | 1k, 18k, 18k |
| Deluxe 5B3 (tweed) | 5Y3GT | +350 | +280 | +250 | 20 uF each | 10k, 10k |
| Code, Cedar Spring | silicon (Rs 110 ohm) | 420 | n/a | n/a | tauSlow 0.12 s | n/a |

### 1.9 Power-stage settings side by side

| Amp | B+ (code) | B+ (sheet) | Bias (code curve) | Bias (sheet) | Screens (sheet) | Feedback beta (code) | Feedback beta (sheet) | Rectifier (code) | Rectifier (sheet) |
|---|---|---|---|---|---|---|---|---|---|
| Twin Sparkle | 480 | 405 to 460 | -53 V / 46 mA | -44 to -52 V | 470 ohm 1 W | -19 dB | -19 dB | silicon, 60 ohm | silicon diodes |
| Tweedle Dee | 335 | not printed | -15 V / 151 mA (comment) | +18 V cathode (5B3/5F1); 36 mA/tube (computed) | none (5000 ohm node) | none | none | 5Y3, 250 ohm | 5Y3GT |
| Four-ten | 335 | 432 (5F6-A) | same as above | -48 V fixed | 470 ohm 1 W | none | 27k + 5k presence | 5Y3, 250 ohm | GZ34 |
| Cedar Blackface | 420 | 415 to 420 (DR) | none (classic shaping) | -35 V | 470 ohm 1 W | -19 dB | -25 dB | silicon, 110 ohm | tube |
| Big Spring | 420 | 460 (Vibroverb AB763) | none | -55 V | 470 ohm 1 W | -19 dB | -25 dB | silicon | GZ34 |
| Quad Ten | 420 | 483 (Hot Rod DeVille) | none | -54.6 V | 470 ohm 1 W | -19 dB | about -21 dB | silicon | bridge |

---

## 2. Revision map

### 2.1 Which revision best matches each existing voicing

| Voicing | Best-matching sheet | Why | Mismatch to know about |
|---|---|---|---|
| Twin Sparkle, Sparkly | AB763 for supply, bias and curve; AA270 for the documented PI numbers | B+ 480 V is closest to AB763 (+460 V); 6L6 med bias -53 V matches AB763 (-52 V); docs quote AA270 PI values | The mix; see 1.1 |
| Twin Sparkle, Edge push / Boutique / Studio | Not circuit-derived (R rows) | | |
| Tweedle Dee Soft, Cranked | 5E3 | Docs and code use the 5E3 tone, cathodes, supply, no feedback | B+ unprinted; screen resistor claim |
| Tweedle Dee Four-ten | Nothing (a tweed Bassman idea on 5E3 settings) | | See 1.3 |
| Tweedle Dee Gritty | 5E3 | | |
| Cedar Spring Blackface | DR AB763 / AB868 (B+ 415/420, bias -35 V, tone stack) | Docs say fitted to twelve DR captures | Feedback (-19 vs -25 dB), rectifier |
| Cedar Spring Spanky | DR AA1172 or AB868 (silverface, 47 pF bright cap) | Bright cap | The sheet's bright cap is milder than the Twin's 120 pF |
| Cedar Spring Cranked | DR with the volume up | No circuit difference | |
| Cedar Spring Big Spring | Vibroverb AB763 or Super Reverb AB763 (the docs name the Vibroverb) | 2 x 6L6, GZ34, +460 V | 420 V supply, 6V6 vs 6L6 tube choice |
| Cedar Spring Quad Ten | Hot Rod DeVille (nearest in the folder; not a Blues DeVille) | Solid state, 2 x 6L6, 4 x 10 | 483 V vs 420 V |
| Sparkle spring | Twin AB763 / AA270 and every blackface | HP 530 Hz | No send HP |

### 2.2 Where revisions differ enough to justify a separate voicing or amp

The user prefers distinct-sounding voicings. Ordered roughly by how different the circuit is from what exists; "needs" lists the code constructs involved.

| Candidate | Sheet(s) | What is electrically different | Needs |
|---|---|---|---|
| Twin AA270 "cold silverface" (separate Twin voicing) | TwinReverb_AA270.md vs AB763 | B+ -55 V (+405 vs +460), preamp D rail +315 V vs +410 V, preamp plates +210 V vs +260 V, PI input 0.01 uF/330k vs 0.001 uF/1M (corner 48 vs 159 Hz, computed), PI loads 47k/47k vs 82k/100k, 68k vs 220k returns, 2000 pF per phase to ground. Same feedback and tone stack | A second `supplyFor` tuple (about 405 V); a preamp stage curve at the lower operating point (the existing arch curves at B=250/220 are close to the AA270's lower plate voltage, so reuse may work); a power-stage input high-pass at 48 Hz instead of 159 Hz; a low-pass for the 2 nF (rough 9 kHz pole, computed, low confidence). The PI is not modelled today, so the HP and LP must be added as filters. Expect: earlier clipping, more low end into the output, darker top, softer feel |
| Twin '65 Reissue | TwinReverb_65Reissue.md | Bridge, 110 uF reservoir (computed), +442 V, 12AX7A, bias adjust range, NFB with 1500 pF, reverb footswitch tap | Little: it is AB763 with a stiffer supply; not worth a separate voicing unless a "stiff Twin" is wanted |
| Super Reverb | SuperReverb_AB763.md | 2 x 6L6GC (half the Twin's current), GZ34, 360 V AC, +460 V, same 1k/4.7k droppers, same NFB (-19 dB), tone stack 0.022 uF cap and fixed 6.8k on the Normal channel (Middle pot on Vibrato), preamp plates +270 V, 4 x 10 | New `supplyFor` (about 460 V, half the Ipk, tube-rectifier Rs); ToneStackTmb variant with C3 = 0.022 uF and 6.8k mid; family 0 feedback already right. Distinct from the Twin: sags like a tube-rectified amp, has a different mid notch |
| Princeton Reverb | PrincetonReverb_AB1270.md | 2 x 6V6, 5U4GB, no choke (1k/18k/18k RC ladder), 340 V AC, light feedback (2700 ohm into 47 ohm), fixed -34 V bias, cathodyne PI, bias tremolo | New `supplyFor`; a new light-feedback family (beta about 1.7 %, -35 dB, computed here); bias-style tremolo; preamp stage three (V3A with NFB). Distinct from the DR by the missing choke (more ripple and a softer supply), the lighter feedback and the lower power |
| Deluxe Reverb '68 Custom | Custom68DeluxeReverb.md | 1.5k feedback resistor (-30 dB vs -25 dB), tone stack 0.022 uF and 24.8k on Custom channel, 3-stage both channels, wet 8 dB lower, 10k grid stoppers, 5AR4 | Feedback family with 1.5k; ToneStackTmb variant; useful if the Cedar Spring should be reshaped around the DR |
| Vibrolux Reverb | VibroluxReverb_AB568.md / AA270.md | 2 x 6L6GC, 5U4GB, +460 V, AB568 hybrid cathode (120/150 ohm) plus grid bias, 820 ohm NFB, tremolo bias (AB568) vs preamp cathode (AA270) | A good Big Spring candidate (40 W 2 x 6L6, 2 x 10) if Vibroverb is not required; different from Super Reverb in the hybrid bias |
| Vibroverb blackface vs brownface | Vibroverb_AB763.md vs Vibroverb_6G16.md | Brown: 7025 PI (not 12AT7), 12AX7 reverb driver, NFB 10k into 4700 ohm (divider (10k+4.7k)/4.7k = 3.1, about 10 dB, computed in the Vibroverb revisions file; beta about 0.32), -62 V bias, B+ +440 V, 16 uF caps with 10k/10k droppers, tremolo by bias modulation with no photocell, 380 V AC | See 2.3 |
| Bandmaster 5E7 | Bandmaster_5E7.md | 2 x 6L6G, 5U4G, +415 to +410 V, 10k then 10k to +332/+280, screens direct, fixed -40 V, 56k + 5k presence loop, cathodyne PI (56k, 1500 ohm + 56k), two channels, bass/treble network, no middle | A new stack (not in ToneStackTmb), tweed push-pull supply with a 5U4G, feedback with presence into the V3 cathode. Distinct from Bassman because of the cathodyne PI and tone network |
| Tweed Bassman 5F6-A | Bassman_5F6A.md | 2 x 5881, GZ34, +432 V, 27k + 5k presence loop, cathode-follower stack (56k, 250 pF, 3 x 0.02 uF, 250k/1M/25k), long-tail PI, -48 V fixed | New tone stack type; push-pull 6L6-class stage curve with tweed-like bias; feedback family with presence; replaces the current Four-ten |
| 6G6-B / AB165 Bassman | Bassman_6G6B.md, Bassman_AB165.md | 6G6-B: silicon, 5881, +450/+428 V, NFB 56k + 25k presence, -54 V. AB165: silicon, 2 x 6L6GC, +425 V, 47k via 0.1 uF into the PI input node, no presence, two stacks (Bass has a DEEP switch), plate-to-PI-plate 220k | Distinct from tweed: different feedback injection (into the PI input node, so current-feedback-like); AB165 is close to the Quad Ten target |
| Tweed Bassman vs JTM-style | Bassman_5F6A.md vs JTM45_tremChassis.md | Same long-tail PI, 27k + 5k presence, cathode-follower stack idea. JTM45: KT66, GZ34 with 360 V AC, +450 V, 32 uF (?), bias network 150k / 56k / 15k; 5F6-A: 5881, 325 V AC, +432 V | If a JTM-style voicing already exists (Plexi_1959 was read), the 5F6-A voicing should differ by tube curve (5881 vs KT66), +18 V lower B+, bias and the OT; tone stack is nearly the same |
| Hot Rod / Blues DeVille | HotRodDeVille.md | Bridge, +483 V, NFB 47k/4.7k (-21 dB, computed) plus presence, 3 x 12AX7A, relay-switched Drive | Closest to Quad Ten; needs an actual Blues DeVille sheet to claim it |

### 2.3 Gaps and what each would need

| Gap | What exists in the plugin | What the sheets give | What it would need |
|---|---|---|---|
| Small amp (Champ / Princeton / Vibro-Champ), single-ended class A | Nothing single-ended; the closest is Tweedle Dee Soft (push-pull) | 5F1, 5F2-A, Vibro-Champ (all printed voltages on 5F1 and Vibro-Champ) | A single-ended power curve (asymmetric clip, even harmonics, no PI), a `supplyFor` tuple (Champ: 340/295/250 V, 5Y3GT; Vibro-Champ: 350/305/265 V), the 22k or 2.2k feedback onto the first stage cathode (beta 6.4 % or 2.1 %), the 5F2-A tone network or no tone, and a small-OT saturation setting (no data on any sheet). The Princeton Reverb adds a push-pull 2 x 6V6 with a cathodyne PI and a different feedback family. The existing engine's "ideal center-tapped push-pull output transformer" does not cover this (MODELED_GEAR.md line 109) |
| Brownface voicing | Nothing | Vibroverb_6G16.md (the only brownface amp sheet; Reverb_6G15.md is an outboard unit) | A family-table entry for weak feedback (10k into 4.7k + 6.8k PI bus); a `supplyFor` tuple (+440 V, GZ34, 380 V AC, 16 uF caps, 10k/10k droppers); the bass cap 0.05 uF (vs 0.047) in the stack; bias-style tremolo (the `Bias` shape exists); 12AX7 reverb driver; the 220k stage-2 load on the Normal channel. A stage curve at the 6G16's lower plate voltages (+220 V stage 1) |
| Super Reverb | None | SuperReverb_AB763.md | See 2.2 |
| Bandmaster | None | Bandmaster_5E7.md | See 2.2 |
| Vibrolux Reverb | None | Two sheets | See 2.2 |
| Separate reverb styles | Seven styles | 6G15 outboard unit; DR '68 (wet 8 dB lower); '65 0.0033 uF (482 Hz); Princeton | An eighth style "6G15" (362 Hz send high-pass, a 50k TONE and a wet/dry mixer) |

---

## 3. Questions and uncertainties

1. Is 480 V for the Twin intended as a loaded or unloaded B+? The AA270 sheet prints +410 V (standby ON?) and +450 V (standby OFF?) on the same wire; its plates/screens are +405 V. If 480 V is a "headroom" setting, keep it, otherwise it is above every Twin sheet. (?)
2. Which Twin do the docs mean by "1972"? The AA270 sheet's rev block reads "1-30-70 (?)", and the docs' PI values are the AA270 ones. Should Sparkle be an AB763-like amp (blackface, stiff) with a separate AA270 voicing, or the reverse?
3. The 6V6 power curve table says bias -15 V, idle 150.7 mA/tube (`pwr_2_med` in StageCurves.h). Fender 6V6 sheets give 24 to 39 mA per tube. Is the comment wrong or is the curve built from a different operating point (a hotter plate/screen voltage)? I did not regenerate the curve. It is used by Tweedle Dee (`defPwr[9]` = 2); Cedar Spring does not use circuit power curves.
4. Does the code use a screen resistor for Tweedle Dee? The docs say 470 ohm 1 W, the 5E3 sheet shows screens straight to the 5000 ohm node.
5. Tweedle Dee's 0.1 uF into 220k: the sheet is 0.1 uF into 1M volume pots; is 220k the volume pot's effective loading at the working setting?
6. Does Cedar Spring need a rectifier choice? The docs say silicon; every DR sheet has a tube rectifier (GZ34 vs 5U4GB differ in internal resistance [theory] and are not distinguished in the code). If a DR is intended, `supplyFor(30)` should get a higher Rs than the Twin's 60 ohm, which it does (110 ohm), but the value is unexplained.
7. Why does model 30 use family 0 (-19 dB)? On the DR family it should be -25 dB. Is the difference deliberate (the twelve DR captures fit better)? The fit notes (kFitLayers) say gain structure and harmonics only, tone left to the EQ.
8. Cedar Spring has no `planFor` (so no circuit stage or power curves) and no `applyComponentVoicing` entry. Was it left out on purpose (it has "No schematics yet")? Now that DR sheets exist, it could use arch 0 (170 V plate, 1.2 V cathode, which matches the DR's plates +170/+180 V and cathode +1.3 V) and cathode corners 1.5k / 25 uF and 820 / 25 uF.
9. Cedar Spring Big Spring: the docs say two 6L6; the voicing comment says two 6V6s with a 15-inch cab. Which one is meant? It decides whether the 6V6 power curve is the wrong tube.
10. Hot Rod DeVille is not a Blues DeVille. The Quad Ten captures are named "Blues DeVille"; the folder has no Blues DeVille sheet.
11. Photocell attack and decay times and the oscillator frequency range of the opto tremolo are on no sheet. The reading gives component values, but nobody has computed the LFO range (SPEED 3M-RA with 100k and the .02/.01/.01 phase-shift caps); the plugin's rates are generic.
12. Where the tremolo acts: Twin/DR/Super Reverb opto on the mixer plate node (vibrato channel only), Princeton Reverb and Vibroverb 6G16 bias modulation, Vibrolux AB568 bias and AA270 preamp cathode, '64 Custom opto into the bias rail, '68 Custom via the PI input impedance (per manual, not traced). Is "Optical/Bias" in the FX meant to follow these?
13. Reverb tank. No sheet names it; CIRCUITS.md calls the Type-4 4AB3C1x "the usual Fender part" which is not read from a sheet.
14. Pot tapers and the exact bias topologies: AA270's bias network (1k (?), 3.3k, 15k, 10k-L pot across the two 68k returns) and the printed values are uncertain; so is the AA270 reverb recovery (220k in series with 0.002 uF vs 220k // 2000 pF).
15. Feedback "dB" is the divider fraction only. Real loop gain depends on the amp's open-loop gain and, for the Princeton/Champ cathode-injection types, on the stage 2 gain, so these are not comparable across topologies. Does `loopGainFor` account for the stage-2 gain for the amps with cathode injection (Vibroverb 6G16, Princeton Reverb, Bandmaster)?
16. Transformer data (Hammond 1750KA in TRANSFORMERS.md) is not used by the model, and Fender OT impedances and saturation (TR3 125A29A for the Twin, 125A1A for the DR) are on no sheet.
17. Super Reverb tone: the Normal channel uses a fixed 6.8k where the Twin has a Middle pot; confirm the Middle knob in the plugin is meant to be absent or approximate for a Super voicing.
18. The Twin Reverb sheets and `TwinReverb_revisions.md` were read from scans; items marked (?) in those readings (the AA270 bias network and filter cap markings, the DR 'node C' value on AA1270) should not be treated as measured.
