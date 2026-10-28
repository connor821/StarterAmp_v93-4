# Twin Reverb revisions: AB763 (blackface) vs AA270 (silverface, dwg 045377 Rev E) vs '65 Reissue (dwg 037627 Rev E)

Sources: `TwinReverb_AB763.md`, `TwinReverb_AA270.md`, `TwinReverb_65Reissue.md` (all read from the scans; see those files for uncertainty notes). "(?)" = not certain; "(computed)" = my arithmetic from printed values, not printed. "Effect" columns are qualitative engineering judgement from the circuit, not measurements.
Reading order for each row: AB763 -> AA270 -> '65. "same" = identical value on both sheets.

## 1. Power supply and operating points
| Item | AB763 | AA270 | '65 Reissue | Effect |
|---|---|---|---|---|
| HV winding | 340 VAC per side | 315 VAC per side | not printed (T3 037610) | Lower rail in AA270 |
| Rectifier | 6 diodes (3 per leg), centre-tapped | same type (6 diodes) | **bridge CR1-CR4 800 V 1 A** | No tube drop in any; bridge uses the full winding |
| Standby switch | in + rail after reservoir | in + rail (same) | **in the negative (return) leg** of the bridge (S3) | None audible; changes switching stress |
| Reservoir | 2 x 70 uF/350 V series + 220k 1 W each = 35 uF (computed) | same | **2 x 220 uF/285 V series + 220k each = 110 uF (computed)** | '65: about 3x the reservoir; less ripple, stiffer short-term B+; sag then mainly transformer regulation |
| Rail after rectifier | +460 V | +410 V / +450 V printed "STANDBY ON/OFF" (meaning not resolved) | +442 V (TP27) | See plates below |
| Plates / OT centre tap | +460 V (before choke) | +405 V | +442 V | AA270 about 55 V lower: lower output power and earlier compression (computed: (405/460)^2 = 0.78, about -1.1 dB maximum power, rough) |
| Screens | +458 V via 470 ohm 1 W | +405 V via 470 ohm 1 W | +440 V via 470 ohm 1 W (Z) | same R, different voltage |
| Choke | TR2 (125C1A) | TR2 | L1 036486 | |
| Dropper B->C (PI plate supply) | 1k 1 W (to +450 V) | **2.2k 1 W (to +380 V (?))** | 1k 1 W R67 (to +428 V) | AA270: bigger drop, PI plates on a softer supply; adds PI-current-dependent sag |
| Dropper C->D (preamp) | 4700 ohm 1 W (to +410 V) | **10k 1 W (to +315 V (?))** | 4.7k 1 W R68 (to +393 V) | AA270 preamp B+ about 95 V below AB763 |
| Filter caps after choke | 20 uF/525 V each (B, C, D) | same (20/525) | 22 uF/500 V each (Z, Y, X) | same order of size |
| Preamp plate voltages | +260 / +270 / +250 / +240 V (V1A / V1B / V2A / V2B) | **+210 V** all | not printed (X = +393 V) | AA270: lower plate V, cathode +1.6 V; stage current about 1.07 mA vs 1.33 mA (computed from 1.6 V / 1.5k and 2.0 V / 1.5k) |
| Preamp cathodes | +2.0 V | +1.6 V | +1.9 to +2.0 V (TP2, TP5, TP12) | |
| 12AX7A vs 7025 | 7025 | 7025 | **12AX7A** (V1, V2, V4, V5) | Different tube type, same triode; little gain change |
| Heaters | grounded centre tap | two 100 ohm to ground | **100 ohm "HUM ADJ." pot R69** | Hum/ripple only |
| Fuse | 2 1/2 A slo-blo | 2 1/2 A | 4 A slo-blo (120 V) | |
| Printed OT load | not printed | "TOTAL LOAD 4 ohm" | two 8 ohm Jensen replicas in parallel = 4 ohm; 85 W min into 4 ohm | AB763 load not printed |

## 2. Phase inverter and output stage
| Item | AB763 | AA270 | '65 Reissue | Effect |
|---|---|---|---|---|
| PI tube | 12AT7 | 12AT7 | 12AT7 | |
| Input coupling (N1 -> top grid) | **.001 uF** | **.01 uF** | **.001 uF** | .001 uF with 1M leak: corner about 159 Hz (computed); .01 uF with 330k: about 48 Hz (computed). AB763 and '65 deliberately roll off the bass into the PI; AA270 passes more bass and drives the output tubes harder at low frequency |
| PI grid leaks | 1M each | **330k** each | 1M each | AA270: lower impedance, less blocking |
| Tail | 470 ohm + 22k + 100 ohm = about 22.57k (computed) | **270 ohm** + 22k + 100 = about 22.37k (computed) | 470 + 22k + 100 = 22.57k | AA270 about 5.4 mA vs 4.4 mA total tail current (computed) |
| PI cathodes | +100 V (X +98 V) | +120 V | +99 V (TP18) | |
| PI plate loads | 82k / 100k 5 % | **47k / 47k** | 82k 1 W / 100k 1 W | AA270: balanced loads, lower PI gain, more drive current into the 68k leaks and 2000 pF |
| PI plates | +245 / +235 V | +245 / +245 V | not printed (Y +428 V) | |
| PI output coupling | .1 uF (400 V) | **.1 uF / 600 V** | .1 uF / 630 V | Rating only |
| Grid return resistors (per phase to bias line) | **220k** 5 % | **68k** | **220k** | AA270: lower leak, more tolerant of grid current, cuts blocking distortion, and loads the PI harder (47k || 68k plate load) |
| Phase-node capacitor | none | **2000 pF per phase to ground** | none | AA270: HF roll-off of roughly 9 kHz (computed rough: PI output impedance about 47k || rp (about 11k, assumed) = about 9k with 2000 pF) -> slightly darker top end and ultrasonic stability |
| Grid stoppers | 1500 ohm each | 1500 ohm each | 1500 ohm each (R46-R49) | same |
| Screen resistors | 470 ohm 1 W | 470 ohm 1 W | 470 ohm 1 W flameproof | same |
| Output tubes | 4 x 6L6GC | 4 x 6L6GC | 4 x 6L6GC | same |
| Bias | fixed -52 V on grid line (diode off a "48 VAC" tap via 470 ohm 1 W, 50 uF; 10k-L pot + 27k) | raw -52 V, wiper -44 V, via "1K (?)" + 3.3k; 10K-L pot with its ends on the two 68k phase returns, 15k to ground (topology uncertain) | CR5 1N4003, raw -65 V; R70 3.3k / R71 10k (BIAS ADJ) / R72 27k; wiper -50 V (TP26); range about -43.5 to -59.7 V (computed) | '65 has a defined adjustable range; AA270 topology not resolved |
| Global NFB | 820 ohm to NFB node (22k/100 ohm), about -19 dB (computed) | 820 ohm, same | **820 ohm || 1500 pF** | 1500 pF corner about 129 kHz (computed): no audio-band change, only ultrasonic phase lead |
| Presence | none | none | none | |

## 3. Tone stack and channel stages
- Tone stack values: **same on all three** (100k slope, 250 pF, .1 uF, .047 uF, Treble 250k, Bass 250k, Middle 10k, 120 pF bright). Pot codes on the '65: Treble "30A", Bass "10A", Middle "10A", Volume "30A" (meaning of the prefix not resolved).
- Mixer, normal channel feed: .047 uF + 220k into N1: same on all three.
- Vibrato channel: V2B plate .02 uF (AB763, AA270) vs **.022 uF** ('65): trivial.
- AA270 V1A/V2A plate loads 100k 1 W (AB763 wattage not printed 1/2 W by default).

## 4. Reverb
| Item | AB763 | AA270 | '65 Reissue | Effect |
|---|---|---|---|---|
| Driver | 12AT7 paralleled, 2200 ohm || 25 uF, +8.6 V | 2200 ohm || 25 uF, +10 V | 2.2k 1 W || 22 uF, +7.8 V | Same |
| Driver plate | +440 V (after TR4 drop), TR4 to B (+458 V) | "+400 V" TR4 to B | TR4 to Z (+440 V) | Voltage tracks B+ |
| Recovery grid | 220k to ground | **220k in series with .002 uF** to ground (?) | 220k to ground | AA270: series RC, minor change to the pan load (qualitative) |
| Recovery coupling to pot | .003 uF | .003 uF | **.0033 uF** | Trivial |
| Reverb return network | wiper -> 470k -> grid node, 220k to ground | same as AB763 | wiper -> **82k + 390k** (472k total, computed) -> grid, 220k to ground; junction taps the reverb footswitch | Same attenuation 0.318 (computed); '65 adds a footswitch tap |
| Mixer / recovery tube | 1/2 7025 | 1/2 7025 | 1/2 12AX7A | |

## 5. Vibrato
| Item | AB763 | AA270 | '65 Reissue | Effect |
|---|---|---|---|---|
| Oscillator plate | +300 V, 220k load to B | +245 V | load 220k to Z | Voltage tracks B+ |
| Modulator plate | +380 V, 10M to B, lamp + 100k to B | +385 V, 10M with **~.012 uF (?) in parallel** | lamp fed from R62 100k from Z; **R63 10M across the lamp**; C40 .022 uF/630 V (?) | Lamp drive and waveform shape differ |
| Phase-shift caps | .02, .01, .0? | same | **.022, .01, .01** | Trivial frequency shift |
| Speed network | 3M-RA + 100k | same | **3M "10C" + 91k** | Slightly different speed range |
| Bias/NB reference | 2.2M to raw -52 V node | same | **R59 1.8M (BOM 2.2M) with R76 270k**, footswitch via R75 10k; NB -8.5 V OFF, -0.35 V ON | New on/off footswitch behaviour |
| Cathode / bypass | 2700 ohm || 25 uF | same | 2.7k || 22 uF; TP32 +1.9 V | |

## 6. Summary of what matters for modelling
1. **AA270 is the "cold" revision**: B+ about 55 V lower at the plates and screens, preamp B+ about 95 V lower, PI plate loads 47k/47k, PI input coupling .01 uF, 68k grid returns and 2000 pF per phase. Expect earlier clipping, tighter bass pass into the output stage, and a slightly darker top end (computed).
2. **AB763 and '65 are electrically close**: same tone stack, same PI values (82k/100k, 470 ohm tail, .001 uF, 1M, 220k returns), same dropper values (1k / 4.7k), 820 ohm NFB. The '65 differences are: 110 uF reservoir, bridge, +442 V plates (-18 V), bias adjust range, 1500 pF across the NFB resistor, vibrato rebuild, reverb footswitch tap.
3. Biggest gain/compression levers (qualitative): plate/B+ voltages, PI loads/leaks/coupling (AA270), reservoir capacitance (the '65), NFB (same 820 ohm on all).

---

# Super Reverb AB763 and other amps vs Twin / Deluxe Reverb output stage and supply

Deluxe Reverb AB763 values below are taken from `DeluxeReverb_AB763.md` (another reader's file, read for comparison only; not re-verified by me). Super Reverb and Hot Rod values from `SuperReverb_AB763.md` and `HotRodDeVille.md`.

| Item | Twin Reverb AB763 | Super Reverb AB763 | Deluxe Reverb AB763 | Effect / note |
|---|---|---|---|---|
| Output tubes | 4 x 6L6GC | **2 x 6L6GC** | 2 x 6V6GT | Super Reverb: half the Twin's current and power |
| Rectifier | solid-state 6-diode | **GZ34 tube** | GZ34 tube | Tube rectifier adds internal-resistance sag (Super Reverb, Deluxe) |
| HV winding | 340 VAC per side | **360 VAC per side** | 330 VAC per side | |
| Plates / OT centre tap | +460 V | +460 V (OT CT +465 V) | +415 V (OT CT +420 V) | Super Reverb plates equal the Twin |
| Screens | +458 V via 470 ohm 1 W | +460 V via 470 ohm 1 W | +415 V via 470 ohm 1 W | |
| Grid stoppers | 1500 ohm | 1500 ohm | 1500 ohm | same |
| Grid returns | 220k 5 % | 220k 5 % | 220k 5 % | same |
| Bias | -52 V fixed (10k pot + 27k) | -52 V fixed (10K-L pot + 27K, 470-1W tap, 25/50 uF) | -35 V (10K-L pot + 10k) | Super Reverb uses the Twin's bias circuit |
| Reservoir | 2 x 70-350 series, 220k each | same | 2 x 16 uF/450 V in parallel (as read) | Deluxe reservoir smaller in absolute terms but drawing much less current |
| Choke / droppers | choke, 1k 1 W, 4700 ohm 1 W | choke, 1k 1 W, 4700 ohm 1 W | choke, 10k, 10k | Super Reverb has the Twin's dropper chain; Deluxe's are much bigger |
| Node voltages (B / C / D) | +458 / +450 / +410 V | +460 / +450 / +410 V | B +410 V (V3 plate); C +325 V; preamp plates about +170-180 V | |
| Preamp plates | +240 to +270 V | +270 V (all four) | +170 to +180 V | Super Reverb preamp hotter than the Twin's |
| Preamp cathodes | +2.0 V | +2.1 V | +1.3 V | |
| PI | 12AT7, 1M leaks, 470 tail, 82k/100k | same values; plates +230 / +230 V, cathodes +106 V | 12AT7 with 1M leaks, 470, 22k, 47 ohm; plates +170 / +180 V | |
| NFB | 820 ohm into 22k/100 ohm node (about -19 dB, computed) | 820 ohm, same node (same -19 dB) | 820 ohm into 22k/47 ohm node (about -25 dB, computed) | Deluxe uses more feedback |
| Tone stack | .1 / .047, middle 10k both channels | .1 / **.022**; **Normal channel: fixed 6800 ohm in place of Middle pot**; Vibrato: 10K-A Middle | .1 / .047 with 6800 ohm to ground, no middle pot | Super Reverb mid-notch differs on both channels |
| OT | TR3-125A29A | TR3 125A9A, 4 speakers, EXT jack | TR3-125A1A (as read) | |
| Reverb driver plate | +440 V | +450 V | +410 V | |

## Tweed Super / Twin vs the blackface amps
| Item | 5F8-A Tweed Twin | 5E4-A Super (6V6GT) | 5F4 Super (6L6G) | Blackface Twin/Super Reverb |
|---|---|---|---|---|
| Output tubes | 4 x 5881 | 2 x 6V6GT | 2 x 6L6G | 4 or 2 x 6L6GC |
| Rectifier | GZ34 | 5U4GA | 5U4G | solid-state (Twin) / GZ34 (Super Reverb) |
| PI | long-tail 12AX7 (47 pF plate-to-plate, 82k/100k) | cathodyne (12AX7), 56k load + 1500 ohm/56k cathode | cathodyne, same | long-tail 12AT7 |
| Screens | 470 ohm 1 W from +395 V | **direct to OT centre tap +385 V** (no screen resistor) | **direct to +410 V** | 470 ohm 1 W |
| Plate / OT CT | +397 V | +385 V | +410 V | +415 to +465 V |
| Bias | -41 V (selenium rectifier, 15k/56k) | -32 V (18k, was 22k; 56k; 100-25) | -40 V (6800 ohm, 56k; 100-25 per layout) | -35 / -52 V, adjustable pot |
| NFB | 56k to PI node, presence 5K | 56k to V3a cathode (1500 ohm): -32 dB (computed), presence 5K | same | 820 ohm to PI node, no presence |
| Droppers | 4700 / 10k | 10k / 10k | 10k / 10k | 1k/4.7k (Twin, Super Reverb) or 10k/10k (Deluxe) |
| Tone stack | 56k slope, .02 uF caps | 1M/1M, .005 and .00025 | same | 100k slope, .1 / .047 |

Notes: the tweed Supers have no cathode resistors or screen resistors and no adjustable bias, so power-stage compression comes from the choke-input tube-rectifier supply with plates and screens on the same node (sag), and from the cathodyne PI. The blackface amps use pre-choke plate feed with screens on the post-choke node.

## Hot Rod DeVille (for reference)
2 x 6L6GC, solid-state bridge, plates/screens +483 V (Z), screen R 470 ohm 1 W, fixed bias -54.6 V adjustable (R82 25k), **NFB 47k from the speaker to a PI NFB node with 4.7k to ground (about -21 dB, computed; PI tail 470 ohm + 6.8k) with presence R67 25k + .1 uF**, droppers 4.7k and 10k to +441 V and +388 V, reservoir 2 x 100 uF/350 V series + 2 x 47 uF/350 V series, three 12AX7A preamp tubes and relay-switched Drive channel (see `HotRodDeVille.md`).
