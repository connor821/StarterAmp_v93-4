# Circuits received, and what was done with them

Status key: **Applied** = the numbers drive the sound. **Partly** = some values used, the rest estimated or left out. **Read only** = read and recorded, nothing in the sound changed. Brand names appear here, as in MODELED_GEAR.md. Nothing below has been listened to against the real gear.

## Pedals

| Circuit | Source | Status | What was taken, and where |
|---|---|---|---|
| Tube Screamer (TS808) | ElectroSmash analysis | Applied | Drive: Screamer. Op-amp stage, 720 Hz corner, 51k + 500k pot, 51 pF, 1N4148 pair, 1k / 0.22 uF tone stage with the 20k pot. Bass, Mid and voicing menus are my additions |
| Boss DS-1 | ElectroSmash | Applied | Drive: Distortion. Booster, op-amp stage, 2.2k / 10 nF into 1N4148 clipping, passive tone network. Not modelled: the booster's treble roll-off, the buffers |
| Klon Centaur | ElectroSmash page, plus the LC_VODA readings still to come | Partly | Drive: Transparent (v93). Gain-pot law, clean and gain paths, shelf, germanium clipping. Estimated: input network, minimum feedback, op-amp bandwidth, clipper values, the shelf, the blend weights (list in MODELED_GEAR) |
| MXR Distortion+ | ElectroSmash | Partly | Drive: Op-Amp Distortion (v93). Gain law, 47 nF, 10k / 1 nF into germanium diodes. Estimated: 1 MHz bandwidth, 3.5 V swing |
| Marshall Guv'nor | ElectroSmash | Partly | Drive: Stack Drive (v93). Two gain stages, red LEDs, 15.3 kHz output low-pass. Estimated: the whole tone section (the page lists no values) |
| MXR MicroAmp | ElectroSmash | Applied | Boost: Clean (v93). Gain law, 4.7 uF, 47 pF, 4.5 V rails. One value (R6 = 2.7k) back-calculated from the page's gain table |
| Dallas Rangemaster | ElectroSmash | Partly | Boost: Treble (v93). 5 nF into 11k, gain 80, 10 nF output coupling. Estimated: the supply limits and the 5 to 47 nF Sat range |
| MXR Dyna Comp | ElectroSmash | Partly | Comp: Dyna (OTA) (v93). Input and output high-passes, the high-frequency lift. Estimated: ratio, attack, release and the whole detector (the page has none) |
| Fuzz Face | ElectroSmash | Read only | The existing Face style already sits inside the page's values (input load, gain range); nothing changed |
| ProCo Rat | ElectroSmash | Read only | The existing Fuzz: Tight style already matched (gain 1 + R / 43.4 ohm, tone 475 Hz to 32 kHz); nothing changed |
| Phase 90 | ElectroSmash | Read only | Stage values given (24k, 47 nF) but not the JFET resistance range, so the sweep cannot be set from the page |
| Vox V847 / CryBaby wah | ElectroSmash | Partly | Wah: Voice, Classic circuit. Sweep 450 Hz to 1.6 kHz. Not applied: the 18 dB peak and Q (the page gives no Q; heel and toe direction is ambiguous) |
| Big Muff Pi (V3) | ElectroSmash | Applied | Fuzz: Wall, Circuit version. Input booster, two diode-limited stages, the tone network solved exactly, output stage. Diode choices are my additions |
| Big Muff, later Russian builds (7B, 7C, Black Russian) | Big Muff Page, schematics sent as images | Applied | Fuzz: Wall, variants B and C. Component values read; the stage model was calibrated to the first circuit's published gains and poles |
| Dream Fuzz (op-amp Muff) | PedalPCB schematic | Applied | Fuzz: Wall, op-amp build. One 1 uF capacitor (about 28 Hz) ignored |
| Collision Drive (Precision Drive style) | PCB Guitar Mania build doc | Applied | Drive: Tight. One capacitor reads as nF in one place and pF in another; I used 47 pF |

Fuzz 2 (v93) is a full copy of the fuzz block, so every fuzz style and every circuit version above is available in both.

Not received or not built: Fuzz Factory (the Factory and Gated styles are built from general knowledge of the circuit, not a schematic), Tone Bender (the three Bender models likewise), Tactical Nuke and the bitcrusher (original designs), Juno-style chorus and the delays (by ear).

## Amps

| Circuit | Status | What was taken, and where |
|---|---|---|
| JCM800 2203-style (and a kit schematic of a 50 W master-volume amp) | Applied | Lead 50 full-circuit code (no longer reachable in v93); the passive stack (250k, 1M, 25k, 33k, 470 pF / 22 nF) and first-stage values in the British amps. The power section's copies were too small to read |
| Fender Twin Reverb (1972) | Partly | American combo passive stack and the stage and power curves. Read but not applied: supply, 6L6 bias path, 820 ohm feedback, phase inverter |
| Vox AC30 (1978 reverb model) | Partly | Stage and power curves (cathode-biased EL84s). The Top Boost stack uses the standard published values, not the schematic (too small to read); Cut control, 47 ohm / 250 uF bias, bridge supply and the inverter are not applied |
| Soldano SLO-100 | Partly | British lead stack (250k, 1M, 25k, 47k, 470 pF / 0.02 uF) and feedback family (39k). Not applied: the cold clipper, late tone stack and master volume in the OD channel |
| Mesa Mark IIC+ | Partly | Creamy Lead: first-stage cathode corner, bright cap, supply. Feedback network noted only. One page is a low-resolution scan |
| Mesa Mark IV | Read only | Recorded; no separate model, and the lead channel does not map to one of my three stages |
| Mesa Mark V | Partly | Supply (448 V, 5.6k x 33 uF) and the feedback network applied to Creamy Lead. Not applied: graphic EQ, reverb, loop, relay logic, simul-class bias |
| Mesa Dual Rectifier (two revisions) | Partly | Modern High-Gain: stacks, first-stage corner, supply, feedback family. Not applied: the power amp's bias switching, the effects loop |
| Peavey 5150 II | Partly | Only its 6L6 power board (2.2k grid stoppers, 100 ohm screens) is used, as the reference for the British-lead feedback family; I did not find its feedback resistor with confidence, so that family is a closest match, not a reading. Preamp, tone stack and phase inverter pages are not applied |
| Friedman BE-100 | Not applied | The copy was too small to read part values with confidence |
| Hand-drawn dual-input 50 W mod (Jose/Arredondo style) | Read only | The mix point could not be read; only the idea of two parallel first stages was used, and it was removed in v87 |

## Still outstanding from you

- LC_VODA readings, to retune Transparent (and the Dyna comp if you have it).
- A clearer drawing of the dual-input mod.
- 5150 II values.
- **HM-2 (received, built in v93):** now the Buzzsaw fuzz style (two gain stages, op-amp, clipper, two-band tone network solved as a circuit). Tone matches the analytic response (about +/-20 dB). Transistor/op-amp details estimated; not listened to.
- **N.E.W. DIY HM-2 (received, mod options built in v94):** the same circuit with the tone split into Mids and High pots, a SOFT switch (the 1N4148 feedback diodes), a HARD switch (the 1N4148 shunt pair) and a BLEND pot to a clean non-inverting copy (gain 2). All in Buzzsaw. The Medusa manual had no schematic, so none of its own controls are built.
- **Roland JC-120/160 service manual (received, not built):** the scan is low resolution; channel 1 reads as a JFET input, passive treble / bass (250k) and mid (10k) stack, volume and a bright switch. Needs a larger scan of the tone stack, distortion channel, chorus and power amp.
- **Fuzz Factory (schematic image received, NOT built into the plugin):** I wrote a transistor-level solver, but it was not stable enough (Silicon mode mostly dead, Germanium latches at full Drive), so Factory keeps its earlier hand-built model. Needs real transistor gains/leakage to finish.
- **Fender 5E3 tweed Deluxe (built, v93):** Low-Watt Combo now uses the schematic's single Tone control (500 pF / 4.7 nF / 1M network solved as a circuit, driven by the Treble knob), its cathode corners (820 ohm / 25 uF, 1.5k / 25 uF), and its supply (5Y3, 345 V plates, 4.7k x 16 uF to the screens). No feedback, as on the schematic (already the case). Estimated: the 12AY7's resistance, the volume pot setting (fixed), rectifier resistance, peak current. Not modelled: the second (bright) input channel and the cathodyne inverter's imbalance.
- **Soldano SLO-100 (more applied, v93):** Saturated Lead takes the lead channel's cathode corners (V1b and V2a both 1.8k / 1 uF) and the supply (497 V, 2 x 200 uF, choke, 10k x 40 uF). Confirmed against the schematic: late stack 250k / 1M / 25k / 47k, 470 pF / 0.02 uF; 39k feedback from the 4 ohm tap, 25k presence with 0.1 uF; 2.2k grid stoppers, 475 ohm screens. Still not modelled: the V2b cold clipper (39k cathode), the two-diode-like clipping at the optos, the FX loop.
- **Marshall 1959 (1970 drawing, applied lightly, v93):** Classic Rock takes the 100 W supply (four EL34, bridge, 10k x 50 uF stage). Confirmed against the schematic: 100k plates, 68k stoppers, first-stage 2.7k / 0.68 uF, stack 250k / 1M / 25k with 500 pF, 0.022 uF, 0.022 uF and a 33k slope, 5.6k grid stoppers, 1k screens. Not applied: the 820 ohm / 250 uF channel-1 path, the 500 pF vs 470 pF stack difference (I judged it negligible), the feedback network (could not read it with confidence).
