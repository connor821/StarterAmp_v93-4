# Based on: the short list (rebuilt in b58)

One page that says, for every part of Starter Amp, what it was aimed at. Real brand and artist names appear in this file and the other docs only. The plugin's own panels, knob names, tooltips and source code use generic names. If you ever release the plugin, keep this file out of the package.

The long version, with the numbers and schematic notes behind each line, is in MODELED_GEAR.md (per block), AMP_SOURCES.md (per amp and voicing) and CIRCUITS.md (schematics received). This file is the quick index. If it disagrees with those, tell me and I will fix it.

**What "based on" means here.** I aimed at a general character or behaviour. Nothing was measured from real hardware, no code or data was copied, and none of it has been listened to against the original. It is not a claim of a match.

**Provenance column**
- **You asked** = you named it.
- **My reading** = I picked the reference from general reputation. Treat as a guess.
- **Own design** = built from a generic idea. No specific unit.
- **Schematic** = component values were read from a schematic you sent (full detail in CIRCUITS.md).
- **Captures** = tone or gain fitted to NAM captures or measurements you sent.

---

## Amps (32 amps; voicing-level detail is in AMP_SOURCES.md)

| # | Amp | Based on | Provenance |
|---|---|---|---|
| 0 | Twin Sparkle | Fender Twin Reverb-style clean | You asked; schematic (1972 Twin) |
| 1 | Chime Thirty | Vox AC30-style | You asked; schematic (1978 AC30) |
| 2 | Brown Eye | Friedman BE-100 / HBE-style | You asked; captures + schematic |
| 3 | Slow Burn | Soldano SLO-100-style | You asked; schematic |
| 4 | Modern Maul | Dual Rectifier-style | You asked; schematic + captures |
| 5 | Acoustic | Piezo / magnetic acoustic amp (the acoustic amps in Neural DSP's Tim Henson and Petrucci plugins) | You asked; own design |
| 6 | Club Crunch | Plexi-style, pushed voicing toward Orange | You asked (the blend is my reading); schematic |
| 7 | Bumble Lead | Dumble-style | You asked; schematic + captures |
| 8 | 5555 Block | Peavey 5150-style; Invective-style (Modern voicing) | You asked; schematic (5150 II power board only) |
| 9 | Tweedle Dee | 5E3 tweed-style Fender | You asked; schematic |
| 10 | Jade Chorus | Roland JC-120-style solid state | You asked; reputation; Distortion stage from the service notes (b39). Preamp re-read in b58, clean stack not applied |
| 11 | Marque Dream | Mesa Mark IIC+ / Mark-series | You asked; schematic + captures. b65: Mark I voicing (position 5) from Mark I captures and sheet |
| 12 | Citrus Doom | Orange-style; Sunn-style stack | You asked; my reading. Sunn Model T sheet received in b58 (stack applied as a Tone Stack entry). b65: Model T voicing (position 5) fitted to Sunn Model T captures |
| 13 | Revved Up | Revv Generator-style | My reading; captures |
| 14 | Digital Dread | Modern hyper-processed plugin amps (Thall-style, Big Bottom-style) | You asked; captures |
| 15 | Master Cranker | JCM800-style; Jose-mod (Friedman Jose-style) | You asked; schematic + captures |
| 16 | Harmonic Haze | Bogner Shiva-style | You asked; schematic |
| 17 | Diesel Fuel | Diezel VH4-style | You asked; captures + service manual; hand-drawn schematic (b58, tone stack applied as a Tone Stack menu entry) |
| 18 | Wreck Stack | ENGL Savage 120-style | You asked; schematic |
| 19 | Power Brawl | ENGL Powerball II-style | You asked; manual + partial schematic |
| 20 | Stolen Fire | Lichtlaerm Prometheus-style | You asked; captures |
| 21 | Hi-Wat Headroom | Hiwatt DR103-style | You asked; schematic |
| 22 | Console DI | Neve 1073-style channel | You asked; manual specs |
| 23 | Plex Appeal | Plexi 100 W-style (with Jose-mod voicings) | You asked; schematic |
| 24 | Matt's Amp | Orange OR120 / GRO 100 / GT120-style | You asked; schematic. b65: Clean 100 (GT1 captures, GT120 sheet) and Tight Voice (schematic, no capture) |
| 25 | Marmalade | Orange Rockerverb-style | You asked; schematic + captures |
| 26 | Eclipse | Modern extreme-metal 100 W head (Nameless-based captures) | You asked; captures (left alone) |
| 27 | Marque Five | Mesa Mark V-style, three channels | You asked; schematic + captures |
| 28 | Dual Thirty | Matchless DC30-style | You asked; schematic |
| 29 | Solid Metal | 90s hybrid and all-JFET metal heads | You asked; schematic |
| 30 | Cedar Spring | Small blackface-style combo | Own design; Twin-based estimates |
| 31 | Triple Blend | Three amps in parallel (Cedar Spring + Bumble Lead + Hi-Wat parts as placeholders) | You asked; placeholder parts |
| 34 | Tweedle Dum | 5F6-A Bassman-style tweed bass amp | You asked (split from Tweedle Dee); schematic + captures |
| 35 | Marque One | Mesa Mark I-style single-channel head | You asked (split from Marque Dream); schematic + captures |
| 36 | Cedar Grand | 40 W two-6L6 reverb combo with a 15-inch speaker | You asked (split from Cedar Spring); captures |
| 37 | Cedar Ten | 4x10 combo with a clean and a gain channel (Blues DeVille-style) | You asked (split from Cedar Spring); captures |
| 38 | Brit Forty-Five | JTM45-style KT66 45 W head | You asked (split from Plex Appeal); schematic + captures |
| 39 | Tweedle Mini | 5 W single-ended tweed (Champ / Princeton-style) | You asked (split from Tweedle Dee); my reading |
| 40 | Jubilant | Silver-jubilee-style cascaded-clipping head | You asked (split from Marmalade); my reading |

Amp-wide controls (Bright, Tube type, Rectifier, Bias, Power Drive, Resonance, Sag, Feel, Deep, Tone Stack) are generic amp-design ideas, my own simple models. The Tone Stack menu holds passive stacks taken from the schematics you sent. Power tube names (EL34, 6L6, 6V6, EL84, KT88) are my rough reading of each tube's reputation.

Considered, not built as amps: RedSeven Leviathan and Driftwood Purple Nightmare (used as the Polished and Boosted voicings), Dover DA-50 (the Mid push voicing leans toward it).

---

## Gain: drive, fuzz and boost

| Part | Based on | Provenance |
|---|---|---|
| Drive: Transparent | Klon Centaur-style (Little Cubes LC_VODA analysis) | You asked |
| Drive: Tight | Horizon Devices Precision Drive-style | My reading |
| Drive: Open | Fulltone OCD-style | You asked |
| Drive: Screamer | Ibanez Tube Screamer family (TS808, TS9, SD-1) + EarthQuaker Palisades-style controls | You asked; schematic analysis |
| Drive: Distortion | Boss DS-1 (plus common LED, germanium, asymmetric mods) | You asked; schematic analysis |
| Drive: Tube Driver | B.K. Butler Tube Driver-style | My reading; own approximation |
| Drive: Op-Amp Distortion | MXR Distortion+ | Schematic analysis |
| Drive: Stack Drive | Marshall Guv'nor | Schematic analysis |
| Drive: Blues Driver | Boss BD-2 Blues Driver | Schematic analysis |
| Boost: Clean | MXR MicroAmp | Schematic analysis |
| Boost: Treble | Dallas Rangemaster | Schematic analysis |
| Fuzz: Wall (Muff) | Big Muff Pi. Versions: Ram's Head, Triangle, Green Russian, Black Russian; Circuit builds from ElectroSmash, Kit Rae's traced schematics, Dream Fuzz, Hizumitas | You asked; schematic |
| Fuzz: Tight (op-amp) | ProCo Rat | You asked |
| Fuzz: Gated (velcro) and Factory | Fuzz Factory | You asked |
| Fuzz: Buzzsaw | Boss HM-2 | You asked; schematic |
| Fuzz: Face | Dallas Arbiter Fuzz Face (germanium and silicon) | You asked |
| Fuzz: Bender | Tone Bender (Early = Mk I, Classic = Mk II, Late = Mk III) | You asked |
| Fuzz: Nuke | The idea behind 3 Sigma Audio Tactical Nuke (wave-shaper) | You asked; own design |
| Fuzz: Crusher | Generic bit and sample-rate crusher | Own design |
| Fuzz: Power Chip | EQD Acapulco Gold | Schematic (build docs) |
| Fuzz: Doom | Life Pedal | Schematic (build docs) |
| Fuzz: Metal Pedal | Merged take on the Lichtlaerm Gehenna and Total Distortion Worship | You asked; descriptions only |
| Fuzz: Distortion + Reverb | A distortion-into-reverb pedal idea | You asked; own design |
| Fuzz options (Starve, Guitar Vol, Buffer, Drift, Hiss, Octave Up) | Behaviours of germanium fuzz pedals | You asked; own design |
| Noise pedal | Tallon Electric x Alpha Wolf "The Noise"; Clash layer after Logan Young Audio "The Gross Button" | You asked; descriptions only |

Read but not applied: Rangemaster-style details beyond the Boost, Phase 90 stage values, Fuzz Face part values (existing model already fits).

---

## Compressor (10 styles)

| Style | Based on | Provenance |
|---|---|---|
| Standard | Empirical Labs Distressor-style (the Dist modes and saturation options) | My reading |
| FET | Generic FET compressor character (fast attack, program-dependent release); ratios follow a measured 1176-type unit (b59). No named unit | Own design, checked against a measurement |
| Opto | Generic optical leveling-amp character (slow, soft knee, two-stage release). No named unit | Own design |
| Variable-mu | Generic variable-mu tube compressor character; attack and release times now follow the Fairchild 660/670 bulletin (b59). No named unit | Own design, checked against the sheet |
| Sustainer | Generic guitar sustainer (high ratio, long release, make-up gain) | Own design |
| Bus | Generic VCA bus compressor character; attack range, ratios and release now follow an SSL-style bus compressor clone's published values (b59) | Own design, checked against the sheet |
| Dyna (OTA) | MXR Dyna Comp | Schematic analysis (filters and HF lift only; the detector is my estimate) |
| Thump | Abasi Micro-Aggressor idea (Bloom, Smack, Clarity, Transformer) | You asked; own design |
| **RMS (VCA) (new, b59)** | **dbx 160 / 160A** (true RMS, hard knee, rate-based release) | **You asked; from the 160A cut sheet (attack and release figures), details of the circuit not modelled** |
| **Opto Sustain (new, b58)** | **Pigtronix Philosopher's Tone** (Sustain, Grit, Treble, Blend, Volume) | **You asked; public descriptions only. Every number is my estimate** |

The Opto Sustain numbers (all estimates): threshold −14 to −54 dB with Sustain, make-up up to +36 dB, 40:1 slope with a 6 dB knee, 2 ms attack, 90 ms fast release into a 0.45 to 0.95 s slow release (the dip-then-swell), Grit as a high-passed hard-clipped layer with a 6 kHz low-pass, Treble as a ±12 dB shelf at 2.5 kHz, Blend as a wet/dry mix.

Ratio menu (eight entries, all styles except Dyna), Detector HP, Saturation: generic compressor ideas.
Multiband Comp (b15, b55 four-band): generic, Linkwitz-Riley crossovers, not matched to any unit.

---

## Modulation, wah, filter

| Part | Based on | Provenance |
|---|---|---|
| Wah (Manual, Auto: Envelope, Auto: LFO) | Generic classic wah | You asked |
| Wah Voice: Classic circuit | Vox V847 / Cry Baby GCB-95 (450 Hz to 1.6 kHz sweep) | Schematic analysis |
| Wah: Envelope Filter | Q-Tron-style envelope filter (circuit not available) | My reading |
| Phaser | MXR Phase 90 | You asked |
| Chorus (I, II, I+II) | Roland Juno-60 / Juno-106 chorus | You asked |
| Chorus: Dimension, Tri-chorus, Ensemble, Detune | Generic chorus types | Own design |
| Chorus: Small Clone (low and high depth) | Electro-Harmonix Small Clone-style chorus (single slow sweep, dark BBD) | My reading; schematic requested |
| Drive: Metal Zone | Boss MT-2 Metal Zone | S-part (service notes read) with estimated tone ranges |
| Flanger, Through-zero | Tape flanging; analog through-zero flangers | My reading |
| Uni-V | Japanese phase-vibrato pedal from the late 1960s | My reading |
| Rotary | Rotating-speaker cabinet (horn and drum) | Own design |
| Harmonic tremolo | 1960s brown-face-era harmonic tremolo circuit | My reading |
| Tremolo shapes (Optical, Bias) | Optical (bulb and photocell) and amp-style bias tremolo | Own design |
| Mod FX Fidelity, eight LFO shapes | Lichtlaerm Nostalgia | You asked; description only |
| Filter (Low/High/Band/Notch, Formant, Resonator) | Generic state-variable filter, vowel band-passes, tuned combs | Own design |
| Lo-Fi warble | Tape and lo-fi pitch wobble | Own design |

---

## Pitch, octave, synth

| Part | Based on | Provenance |
|---|---|---|
| Pitch pedal (Position, Octave, Dive bomb, Bend, Custom) | DigiTech Whammy-style, and the "Wow" pedal in Neural DSP's Gojira plugin | You asked |
| Octaver | The octave pedal in Neural DSP's Gojira plugin; analog octavers | My reading |
| Harmonizer (Interval / Scale) | Intelligent-harmony pitch pedals | Own design |
| Chord-following harmonizer | Polyphonic harmonizers (spectral shifter, multi-note detector) | Own design |
| Transpose / formant | Generic pitch shift with formant control | Own design |
| Guitar synth, 7 and 8-string range | Generic | Own design |
| Ring mod, AM | Generic ring modulator | Own design |

---

## Delay

| Part | Based on | Provenance |
|---|---|---|
| Delay types Digital, Analog, Tape, Lo-Fi | Generic | You asked (lo-fi and tape warble) |
| Tape Sustain | Single-head tape echo (Echoplex idea) | My reading |
| Three-Head Tape | Multi-head tape echo (Space Echo idea) | My reading |
| Chorus Bucket | Bucket-brigade delay with chorus (Deluxe Memory Man idea) | My reading |
| Studio Tape | Clean studio tape slap / echo | My reading |
| Dub Echo | Classic dub tape and tube echoes | My reading |
| Memory Man | Electro-Harmonix Deluxe Memory Man (analog bucket-brigade echo with chorus/vibrato) | My reading; schematic and/or sound sample requested |
| Duck, Hold (Freeze / Infinite), Multi-tap | Ducking delays; freeze footswitch modes; multi-head and tap patterns | My reading |
| Groove / Feel / Accent | Soundtoys EchoBoy-style controls | My reading |
| Echo EQ with Decay | Three-band echo tone control (in the loop or at the output) | My reading |
| Saturation (Tape, Tube, Diode, Transformer) | Generic soft-clip curves | Own design |
| Post delay (Ping-Pong, Dual, etc.) | Generic stereo delays | Own design |
| Pitched delay feedback (b53) | Pitch-shifted feedback echoes | Own design |

---

## Reverb

| Part | Based on | Provenance |
|---|---|---|
| Shimmer | Valhalla Shimmer | You asked |
| Black Hole + Gravity | Eventide Space / Blackhole (public description only) | You asked |
| Reverse, wet tremolo, wet drive, duck release, Focus / Purity | Feature set of Aberrant DSP's Lair | You asked; own DSP |
| ModEcho, Mangled | Echo-into-reverb, heavily fed-back reverb | Own design |
| Classic, Room, Hall, Plate, Ambient, Cathedral, Soft Plate, Tumble, Abyss, Chorale, Nonlinear | Generic algorithm types | Own design |
| Spring, Spring (tank), return styles | Twin Reverb (Sparkle), AC30 (Chime), Matchless Chieftain (Tight), Orange (Full) returns | Schematic |
| IR Reverb | Convolution with an impulse response | Own design |

---

## Cab, mics, rooms

| Part | Based on | Provenance |
|---|---|---|
| Speaker: Modern Tight | Celestion Vintage 30 | You asked |
| Vintage Crunch / Rock Bite / Smooth Vintage / Chimey Alnico | Celestion Greenback / G12T-75 / Creamback / Alnico Blue | My reading |
| Vintage American | Jensen C12N | My reading |
| Hi-Fi Full | Electro-Voice EVM12L | My reading |
| Full-Range Open | Lichtlaerm LCFR-150 (fitted to three SM57 IRs you sent) | You asked; measured |
| Heavy, Studio, Classic, Compact, Rhythm speakers | Fitted to cab IR packs you sent (MDL, Zila, JST, Ugritone, Little Cubes, Petrucci blends) | Measured |
| Modern Scoop, Tight Ceramic, Dark Thump, Hi-Def Light Cone, Smooth Lead, Mid Forward | Generic modern-metal tunings | Own design |
| Mic: Dynamic Bright | Shure SM57 | You asked |
| Dynamic Full / Presence | Sennheiser MD 421 / e906 | My reading |
| Ribbon Smooth / Open | Royer R-121 | My reading; measured |
| Condenser Detailed / Small / Large | Neumann U87 / KM184 / AKG C414 | My reading; measured |
| Dynamic Long Throw / Hyper / Wide | EV RE20 / Beyer M201 / Audix i5 | Measured |
| Oversized 4x12 ported | ML Sound Lab "Reflex"-style cab | You asked |
| Cab blocks A to D, alignment | Multi-mic cab layering in amp sims | You asked |
| Speaker EQ, Reso, Phase, Dynamics, Export as IR | Bogren Digital IRDX idea | You asked; own design |
| Rooms (Original, Studio, Large studio, Living room) | See MODELED_GEAR.md, cab section | Not re-checked in this rebuild |
| NAM player | Neural Amp Modeler file format | You asked; own implementation |

---

## EQ, post FX and tools

| Part | Based on | Provenance |
|---|---|---|
| Post EQ (9 bands) | Neural DSP Archetype Nolly-style 9-band EQ idea | My reading |
| Tone matching | Tone-match features in other amp software | Own design |
| DI Match | JST DI Match idea | Own design |
| Doubler | Neural DSP-style doubler | You asked |
| Widener (Classic, Desktop, Rack) | Korneff Audio Wow Thing | You asked |
| Transient shaper (b53, b54, Body b56) | Generic transient designers | Own design |
| Multiband saturator (b55) | Generic | Own design |
| Gate / Expander | Standard gate | Own design |
| Tremolo, Wobble, Swell, Granular | Generic (shoegaze / soundscape request) | You asked for the features |
| Looper, Riff recorder | Generic | Own design |
| Console DI amp | Neve 1073-style channel | You asked |
| Pedal art, amp illustration | My own drawings in the style of boutique-pedal art (EQD and Walrus Audio as style reference) | You asked |

---

## Factory presets (what each was aimed at)

Djent rhythm: Meshuggah, Periphery, Animals as Leaders, Monuments. Singing leads: Plini, Dream Theater / Petrucci. Prog rhythm: Dream Theater, Haken, Tool. Pitch-pedal and octave sounds: Gojira (as you asked). Groove metal: Gojira, Lamb of God. Sludge and post-metal: Mastodon, Baroness, Neurosis, Isis. Melodic death: At the Gates, Dark Tranquillity, Opeth, Insomnium. Blackgaze: Alcest, Deafheaven. Ambient cleans: Tool, Intervals, Sleep Token. Details and mapping per preset are in MODELED_GEAR.md.

---

## Things I am least sure of (worth correcting)

- FET, Opto, Variable-mu, Sustainer and Bus compressor styles have no named unit behind them. If you want any of them tied to a specific unit, tell me which.
- Orange amp mapping for Citrus Doom is reputation only.
- Revved Up and Digital Dread references come from web write-ups I could not check against the players' rigs.
- The Philosopher's Tone figures (thresholds, release times, Grit chain, Treble corner) are my estimates from public descriptions, not measurements.

## EQ (b59)

| Part | Based on | Provenance |
|---|---|---|
| Program EQ | Pultec EQP-1A program equaliser (low boost and attenuate, high peak boost with bandwidth, high attenuate) | You asked; manual and curve sheet read by eye, shapes approximate; Warmth is my own saturation |
| Mic distance law | Pressure-gradient microphone proximity effect, fitted to datasheet curves at two distances | Physics plus sheets |
| Crush Limiter (comp style, b62) | Soundtoys Devil-Loc (Crush, Crunch, Darkness, Mix, Slow / Fast) | You asked; public manual; every number is my estimate |
| Tape boost mode and Cassette voicing (b62) | Tascam 424-type cassette four-track channel | You asked; general knowledge of the machine, not measured |
| Frequency shifter / ring mod (Mod FX, b62) | Electro-Harmonix Ring Thing, Catalinbread Bicycle-style shifters | You asked (gap survey); standard single-sideband method |
| Vocoder (Mod FX, b62) | Electro-Harmonix V256 and Iron Lung-style vocoders | You asked (gap survey); standard band-pass-bank vocoder |
| Feral (fuzz style, b62) | Self-oscillating bias-modulated fuzzes (Antichthon, Teaser Stallion-style) | You asked (gap survey); my own circuit idea, no schematic |
