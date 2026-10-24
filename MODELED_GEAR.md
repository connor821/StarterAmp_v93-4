# Modeled gear: what each part is inspired by

This file is for your own reference. The plugin itself (its panels, knob names, tooltips, parameter names, source comments and README) uses generic names only, so this list is the one place the real names appear. If you ever release the plugin, keep this file out of the package, and do not use these names in the product name, the UI or the marketing (they are other companies' trademarks).

**How to read this.** Everything here is a hand-built approximation of a general character, tuned by ear. Nothing was measured from real hardware or copied from another product's code or data, and I have not listened to the plugin myself. "Inspired by" means "I aimed at roughly that kind of sound or behaviour". It does not mean it matches the original.

Two columns of provenance are used below:
- **You asked** = you named it in the project (this is also recorded in the project notes).
- **My reading** = I picked the reference myself, from the general reputation of the thing. Treat these as guesses and tell me if one is wrong.

Sorted by block. Sections: Amps; Gain: fuzz, overdrive and boost; Compressor; Mod FX, wah, phaser, chorus, filters and lo-fi; Pitch, octave, harmonizer, synth and noise; Delay; Reverb; Cab section; EQ and tone matching; Post FX: doubler and widener; Other effects and tools; Factory presets; General notes.

# Amps

## Amp models

| Plugin name | Inspired by | Provenance |
|---|---|---|
| Twin Sparkle (sparkly) | Fender-style clean combo | You asked |
| Chime Thirty (class-A) | Vox AC30-style amp | You asked |
| Brown Eye (tight) | Friedman HBE-style amp | You asked |
| Slow Burn (smooth) | Soldano SLO-style amp; the Voicing switch's lead voicing is a Petrucci-style lead tone | You asked |
| Modern Maul (scooped) | Dual Rectifier-style amp; the Voicing switch is modelled on the idea of the voice switch in Neural DSP's Archetype Misha Mansoor (what that switch really does is not documented, so the actual voicing is my own take) | You asked |
| Acoustic (piezo to acoustic) | The acoustic / piezo amps in Neural DSP's Tim Henson and John Petrucci plugins | You asked |
| Club Crunch (crunch) | A Marshall Plexi-style amp (the "plexi" you suggested); the pushed voicing leans toward an Orange-style amp's thicker, rounder mids (the "orange" you suggested). A blend of the two ideas, my own tuning | You asked (plexi / orange, with a question mark, so the exact blend is my reading) |
| Bumble Lead (smooth, singing) | A Dumble-style overdrive amp; the Voicing switch is Jazz (off) and Rock (on). My own tuning, plus (v0.2) cathode values, bright cap, overdrive-channel coupling and power-supply values read from Dumble-style schematics, and a Dumble-style passive stack in the Tone Stack menu | You asked |
| 5555 Block (raw to modern) | A Peavey 5150 "block letter"-style amp (Voicing off, Raw) and an Invective-style amp (Voicing on, Modern). My own tuning | You asked |
| Tweedle Dee (spongy breakup) | A small tweed-style Fender combo (Voicing off = Soft; on = Cranked, wide open). My own tuning | You asked (tweed) |
| Jade Chorus (solid-state) | A Roland JC-120-style solid-state clean (Voicing off = Glass; on = Warm). My own tuning | You asked (JC) |
| Marque Dream (tight, mid-hump) | A Mesa Boogie Mark-series-style amp. Rhythm (off) leans toward the tight, scooped-low-mid rhythm sound of the later Marks (Mark III / IV); Lead (on) leans toward the classic Mark IIC+ mid-hump lead; Vintage (third, v60) is modelled on the early Mark I (lower gain, thick creamy mids, smaller sweeter lows). My own tuning, from general reputation | You asked (Mark; the era mapping is my reading) |
| Citrus Doom (thick, loose) | An Orange-style amp (Voicing off = Stoner) and a Sunn O)))-style stack (Voicing on = Wall). My own tuning | You asked (Orange / Sunn O))) ) |
| Harmonic Haze (rich, mid-focused) | A Bogner Shiva-style amp (Lead, Crunch and Clean = its lead channel at three gain settings and its clean channel; Boost = the lead boost). Read from the Shiva schematic and manual you sent; the rest is its reputation (rich, mid-focused, smooth saturation). My own tuning | You asked (Shiva) |
| Diesel Fuel (tight, cutting) | A Diezel VH4-style amp (Lead and High gain = its channels 3 and 4, Crunch = channel 2; Open = the "less compression and more presence" change in the service manual). Only the service manual (bias, tube layout, tube types) was usable, no schematic, so almost everything is reputation. My own tuning | You asked (VH4) |
| Wreck Stack (huge, very tight) | An ENGL Savage 120-style amp (6550 power tubes; Lead, Crunch and Clean = its lead, crunch and clean channels; Smooth = the Rough / Smooth switch's smooth side). Read from the Savage 120 schematic: supply rails, cathode networks, feedback and presence network. My own tuning | You asked (Savage) |
| Power Brawl (punchy 6L6) | An ENGL Powerball II (E645II)-style amp (Scooped = Lead channel with Middle, Mid-boosted = the Middle-boosted voicing, Crunch = its crunch channel, Bottom = the Bottom switch). Built from the owner's manual (controls and voicing descriptions). The schematic scan was too small to read values from. My own tuning | You asked (Powerball) |
| Cutaway (growl, flexible) | A Lichtlaerm Prometheus-style amp (100 to 140 W, EL34 as shipped; Lead and Sludge = the lead channel's high and low gain, Clean = the warm clean, Brilliant = the chimey clean). From the shop page's description only (no schematic, no manual). The mid range is placed lower (about 1.5 kHz) than the other high-gain amps because the maker says it sits "in a different range"; that is my interpretation. My own tuning | You asked (Prometheus) |

The Modern Metal controls (Tight, Mid, Sag, Chug Gate and so on), the extra top-of-knob gain and the Amp Focus low-tuning tightening are my own additions, with no single source. Amp B uses the same electric amps (and, from v41, Club Crunch).

The amp realism controls (Bright, Tube type, Rectifier, Bias, Power Drive, Resonance, Sag) are generic amp-design ideas (bright cap, power-tube class and bias, tube versus silicon rectifier sag, power-amp saturation, speaker resonance / presence interaction). They are my own simple models, not copies of any product.

**Power tube types.** The plugin shows only the tube names (EL34, 6L6, 6V6, EL84, KT88; the preamp tubes are 12AX7, 12AT7, 12AY7). Each is my rough reading of that tube's general reputation, tuned by ear, not a measurement: EL34 = the crunchy mid-forward British sound (the Marshall / Orange direction); 6L6 = the tight, clean, scooped American sound (Fender / Mesa-style); 6V6 = a small, soft, early-breakup American amp (a small Fender or Supro-style amp); EL84 = the bright, chimey, compressed Vox-style sound; KT88 = the big, tight, high-headroom sound of large Hi-Fi-derived amps (Ampeg / big Marshall-style heads). Each tube changes the power-stage drive, sag, tone-stack offsets and presence. My guesses, so tell me if one is wrong.

**Neural amp captures.** The ".nam" format is the open neural amp capture format of the Neural Amp Modeler project (Steve Atkinson). The plugin's player is my own implementation of the published file format (WaveNet and LSTM captures), checked against the project's open-source engine (NeuralAmpModelerCore, which I used only as a test reference and did not include). Models you download are the work of their makers and have their own licences. The plugin UI only says "NAM" and ".nam".

### Voicings: third position

| Amp | Third voicing | Modelled on (my own tuning, from general reputation) |
|---|---|---|
| Clean | Boutique | A Matchless / Two-Rock-style boutique clean: fuller low mids, tighter lows, more compression and a touch-sensitive chime |
| Chime Thirty | Normal | The Normal channel of a Vox AC30-style amp (no top boost): darker, rounder, thicker |
| Hot-Rod | Brown | The "Brown Eye" voicing of a Friedman HBE-style amp: thicker, saggier, mid-rich, looser |
| Slow Burn | Crunch | The low-gain crunch channel of a Soldano SLO-style amp: open, dynamic, much less compression |
| Modern Maul | Vintage | The spongy Vintage mode of a Dual Rectifier-style amp: tube-rectifier sag, looser and fatter lows, softer attack |
| Club Crunch | Jumped | Both channels of a vintage two-channel head linked (a "jumped" Marshall-style Plexi/Super Lead sound): fuller low mids, more gain and sag |
| Master Cranker | Stock | A Marshall JCM800-style 80s hard-rock amp (the old Club Crunch "Master volume" voicing, unchanged) |
| Master Cranker | Modded | The "Jose mod" idea: the Jose Arredondo hot-rod modification of a JCM800-style amp (an extra gain stage; tighter, smoother, more saturated). Friedman's Jose Arredondo amp is the commercial version. Reference supplied by the user: Rabea Massaad demo video (the page only describes it as "high-gain with rich harmonics and strong tonal control"; I could not hear it, so the voicing was set from the general description of the mod, not matched to that video) |
| Master Cranker | Cranked | A JCM800-style amp with the master volume wide open: power-tube clipping, looser and saggier |
| Bumble Lead | Lead | The boosted lead mode of a Dumble-style amp: more compression and sustain, a vocal mid hump |
| 5555 Block | Crunch | A low-gain crunch channel in the style of the Peavey 5150 family: tight lows, much less saturation |
| Tweedle Dee | Four-ten | A 4x10 tweed Fender Bassman-style voicing: more headroom, bigger lows, slightly scooped low mid, brighter top, less sag (retuned in v76 to separate it from the Soft voicing) |
| Jade Chorus | Funk | A tight, percussive, scooped solid-state voicing (a JC-120-style amp played for funk) |
| Marque Dream | Vintage | An early Mesa Boogie Mark I-style sound (see the Marque Dream row above) |
| Citrus Doom | Bass stack | A bass-amp-style stack (Ampeg SVT / Sunn Model T territory): massive low mids, cleaner top, huge headroom |

### Revved Up, Digital Dread and the artwork

| Part | Inspired by | Provenance |
|---|---|---|
| Revved Up (tight, modern) | A modern boutique high-gain head: Revv Generator-style (tight, articulate, clear, a firm low end, sagging a little on held notes). Voicing 1 "Tight" is the main high-gain channel; "Lead" is the boosted lead mode; "Crunch" is a lower-gain open channel. Also nudged toward the general reputation of the heads in John Browne's (Monuments) rigs: Equipboard lists, among others, a Dual Rectifier, a 5150III, an Ironheart, a Black Spirit, a Diezel VH4 preamp module and a Framus Cobra, and his STL Tones signature pack is described as built across fifteen amps for modern progressive metal. The pack does not say which amps, so I did not try to match any one of them. | You asked (Revv Generator, John Browne / STL Tones); the rest is my reading |
| Digital Dread (brutal, digital) | A modern digital amp sim for extreme styles. Voicing 1 "Thall": ultra-tight thall and djent in the manner of Humanity's Last Breath (very high gain that stays clear, a deep narrow scoop, heavy "subby" lows; the Nail The Mix write-up of that tone describes a Neural DSP Nameless amp block, a boost and EQ in front, early compression and a dynamic cut on the low-end bloom). Voicing 2 "Industrial": harsh, gritty, fuzzy, sound-design-friendly guitar in the manner of the DOOM soundtrack (Mick Gordon) and the synth-driven metalcore of Vianova (a Berlin metalcore band; its press describes a synth-driven sound). Voicing 3 "Cavern": huge, slow, dark, drop-tuned slam and doom. | You asked (Humanity's Last Breath, Mick Gordon / DOOM, Vianova, a thall amp); the voicing split is my reading |
| Amp illustration (AMP page) | A generic amp head on a 4x12 cab, coloured by the selected amp's look. No product artwork is copied | My own drawing |
| Pedal art (enclosure bands, motifs) | The look of boutique-pedal art: bold flat enclosure colours with simple line-art illustrations (an eye, sun, moon, waves, mountains, spiral, dots, lightning). Nine small motifs I drew myself with paths. No existing artwork, logo or character is copied | You asked (EQD and Walrus Audio pedal art as the style reference); the drawings are my own |

Sources for the research notes above: Equipboard's John Browne page (user-contributed gear list), the STL Tones John Browne ToneHub pack page, the Nail The Mix write-up "Humanity's Last Breath guitar tone" and the Arising Empire / Hold Tight press pages for Vianova. I could not check any of them against the players' actual current rigs.

**Update (you told me, from the John Browne STL preset list):** the preset list names the amp, the boost or overdrive and the cab for each preset. The amps in it include ones the plugin already has elements of (Soldano-style, Kraken-style and 5150-style) and others it does not model directly: a Red Seven Leviathan, a Hughes & Kettner and a Driftwood Purple Nightmare. I have not seen the list myself (the STL pages do not show it), so this is your description, not something I checked. Revved Up is therefore a blend of a Revv Generator-style channel and general modern high-gain character, and is not tuned to any of these amps in particular.

### Voicings: fourth position

Polished (Revved Up) is inspired by the RedSeven Leviathan's clear, KT88-style modern high gain. Boosted (Master Cranker) is inspired by the Driftwood Purple Nightmare's built-in screamer-style boost. Mid push (Hot-Rod) leans toward the Dover DA-50's mid-forward EL34 character. The rest are generic variations on each amp's own voicings. All are rules of thumb, not measurements of those amps.

### Amp-wide switches (no specific product)

| Control | What it is in real amps | Source |
|---|---|---|
| Tone Stack (As voiced / Neutral / passive stacks; v87 removed the two classic American and British entries) | The two classic tone-stack families: the American-style scooped-mid stack (Fender-style) and the British-style mid-forward stack (Marshall-style), plus a near-flat option | You asked ("tone-stack type") |
| Feel (Loose / Normal / Tight) | The negative-feedback loop of the power amp (the same idea as the Presence and Resonance controls on many amps) | You asked ("negative-feedback tight/loose") |
| Deep | The Deep or Fat switch found on several amps | My reading |

### Amps considered but not built

Descriptions are from reviews and maker pages, not from listening. The user also supplied the preset list (amp + boost/OD + cab per preset). John Browne's "Red Seven Leviathan" demo video (2021) lists the amp as a KT88-driven two-channel all-tube head.

| Amp | What it is | Fit |
|---|---|---|
| RedSeven Leviathan | KT88-driven two-channel modern high-gain head | Close to Revved Up; better as a voicing than an amp |
| Driftwood Purple Nightmare | 6L6/KT88-style boutique head, pick-responsive, built-in screamer-style boost | Between Precision Crunch and Lead; a voicing |
| Dover DA-50 | EL34-style 50 W British hot-rod, switchable boost stages, mid-forward | A British hot-rod voicing; not built yet |

## Circuit Amp (v79)

- **Lead 50:** a Marshall JCM800 2203-style 50 W master-volume head, solved from its schematic with Koren-type triode and pentode models: V1b (with the volume pot and bright network), V1a, V2a, the cathode follower, the 33k / 22 nF / 470 pF tone stack with 250k treble, 1M bass and 25k mid pots, the 1M master volume, the long-tail phase inverter, two EL34s in push-pull into a 3.4k:8 ohm output transformer, the presence/negative-feedback network, and a supply with reservoir sag. Idle operating points match the published voltage table (V1b plate 196 V against 190 V, cathode 1.61 V against 1.63 V). Schematics received but not yet modeled: Peavey 5150 (EVH), Friedman BE-100, Mesa/Boogie Mark IIC+ preamp and Simul-Class power amp, Soldano SLO-100, Fender Twin Reverb (1972, no MV switch), Vox AC30 (reverb model, 1978); Dual Rectifier schematic still to come. Known approximations: power tubes other than EL34 use approximate curves; the output transformer is an ideal center-tapped model with magnetizing inductance and leakage low-pass; grid-current and screen behavior are simplified.

## Stage model (v80)

The circuit-derived stage curves come from static solutions of the tube equations (Koren triodes and pentodes): preamp stages with 100k plate load (47k for the 12AT7) and cathode resistors of 820 ohm to 2.7k at 220 to 350 V, with grid conduction through a 68k source; push-pull power stages with an ideal center-tapped output transformer, 40 ohm primary resistance and grid conduction through 60k. Amp model to stage type: Clean, Jade Chorus (not used), Acoustic (not used): blackface-style 250 V / 1.5k; Chime Thirty and Tweedle Dee: 220 V / 2.2k cold class-A stage; Hot-Rod, Club Crunch, MV Rock (and the first stages of the Saturated, Creamy and Doom amps): Plexi/JCM-style 300 V / 2.7k crunch stage; the later stages of the lead and high-gain amps: 350 V / 1.5k hot stage; 5555 Block and Precision (last stage): 320 V / 820 ohm cascade stage. Power tube per amp model: EL34 for Hot-Rod, Slow Burn, Club Crunch, Creamy and MV Rock; 6L6 for Clean, Modern, Boutique, 5555 Block and Precision; EL84 for Chime Thirty; 6V6 for Tweedle Dee; KT88 for Citrus Doom. These are archetypes, not circuit models of those exact amps.

## What the schematics told me (v81)

Offline circuit work from the schematics you sent, and where it went into the plugin.

- **Passive tone stacks (applied, v81).** Component values as read from the schematics. American combo (Twin Reverb 1972): 250k treble and bass pots (audio taper), 10k mid, 100k slope resistor, 250 pF / 0.1 uF / 0.047 uF, driven from a gain stage's plate (about 38k source) and into a 1M volume pot. British 50 W (JCM800 2203-style): 250k treble, 1M bass (audio taper), 25k mid, 33k slope, 470 pF / 22 nF / 22 nF, driven by a cathode follower. British lead (SLO-100): 250k treble, 1M bass, 25k mid, 47k slope, 470 pF / 0.02 uF / 0.02 uF, driven by a cathode follower. American lead (Mark IIC+): read as the Twin values (250 pF, 0.1 uF, 0.047 uF, 250k, 250k, 10k) with a lighter 470k load; the slope resistor and exact caps are not legible in the copy I have, so this one is the least certain. Chime (AC30 Top Boost): the standard published values for that type of stack (50 pF, 22 nF, 22 nF, 1M, 1M, 10k, 56k), which a closer reading of the 1978 schematic in b33 confirmed; its Cut control is not modeled.
- **Stage and power curves (applied, v80).** The preamp stage types (plate load 100k, cathode resistors 820 ohm to 2.7k, supplies 220 to 350 V) and the push-pull power curves come from these amps' stages. The Twin's 6L6 pairs, the AC30's cathode-biased EL84s, the SLO-100's 6L6GCs at 495 V and the JCM800's EL34s at 365 V are the reference tubes for the power curves.
- **Read but not yet applied.** Twin Reverb: supply of four silicon diodes into 20 uF stages (2.2k and 10k dropping resistors; B+ about 480 V under no load), 6L6GC pairs with 470 ohm / 1 W screen resistors (the cathodes are grounded, fixed bias; corrected in b33), 820 ohm feedback from the speaker tap, 12AT7 phase inverter with 330k / 47k plate and cathode networks. AC30 (1978): four EL84s, cathode bias with a shared 47 ohm / 250 uF, a bridge rectifier (4 x 1N4007), no global feedback, a Top Cut control after the phase inverter, a 12AX7 long-tail inverter. SLO-100: four 6L6GC at 495 V, 4.4k ohm primary, 39k negative feedback (about 2.6 times the JCM800's), presence 25k with 0.1 uF and 4.7k, a cold-clipper stage in the OD channel, parallel clean and OD channels with a late tone stack and master volume. JCM800 power section and 5150, BE-100 and IIC+ power amps: the copies I have are too small to read part values with confidence.
- **Fixed in v81.** The Bass knob of the v79 Circuit Amp worked backwards (up cut the lows); it is corrected, and the same stack engine is used for the new passive stacks.

## Dual Rectifier schematics (v84)

Read from the preamp and power amp images, and from a closer crop of the tone stack of a later revision. Stacks (applied, v84): 250k treble (220k in the later revision), 1M bass, 25k mid (22k later), 47.5k slope, 0.02 uF caps, treble caps of 680 pF (Red channel) and 600 pF (Orange; 500 pF in the later revision). I used the later revision's values for the two new stack entries, with a cathode-follower source of about 1k (assumed). Power amp (read, not yet applied): four power tubes (the bias note reads "6L6 / EL34", so the type is not certain) with 1.5k grid stoppers, 1k / 2 W screen resistors, a 12AX7 long-tail inverter at about 280 V, cathode-bias supply lines at about -51 V and -39 V, and a feedback network from the 4 / 8 ohm taps through two 47k resistors with a 25k presence pot (low-resolution image, so treat the 47k as roughly right). That is close to my British lead feedback family (39k), so no new family was added. Supply filter values are not legible.

## Power stage inside its feedback loop (v86)

The loop gain (open-loop gain times the feedback fraction Rk / (Rk + Rfb / feel)) is stored per family and Feel setting: American combo 1.4 / 2.8 / 4.7 (loose, normal, tight), British 50 W 0.8 / 1.7 / 3.0, British lead 1.8 / 3.7 / 6.1, light feedback 0.3 / 0.6 / 1.1, no feedback 0. They come from the same open-loop gains and resistor values as the response table, so they carry the same uncertainty (the open-loop gains are estimates). The presence network changes the loop gain with frequency, which the response table already covers, so the loop here uses the low-frequency value. Measured with the preamp turned down so the power stage clips: tight feedback gives about 1 dB more non-fundamental energy than classic at the highest input and loose about 0.5 dB less; the effect is small because the preamp stages clip first at most settings.

## Dual voice (v85, removed in v87)

The control was removed in v87 because its effect was subtle and overlapped the gain and stage controls; the schematic reading below is kept for reference.

From a hand-drawn schematic of a dual-input modification of a 50 W master-volume amp: both halves of the first tube are used as separate gain stages with identical cathodes (2.7k / 0.68 uF) and 100k plate loads, mixed through 1M pots with Vol I and Vol II controls and switched loops. I could not read where the mix sits relative to the second stage, so the plugin only takes the idea: two parallel first stages at different gains, blended by audio-taper weights. The low-gain voice at 0.35 of the full gain is my choice. Measured effect is small (see CHANGELOG v85); a clearer drawing would let me place the mix properly.

## 50 W British schematic check (v84)

A kit schematic of a 50 W master-volume British amp, read after the fact, agrees with the values I used: first stage 2.7k / 0.68 uF cathode and 100k plate; 68k grid stopper and 1M leak; 0.022 uF couplings into 470k; stack 250k treble, 1M bass, 25k mid, 33k slope, 0.022 uF caps (the treble cap reads 500 pF, I use 470 pF); phase inverter with 82k / 100k plate loads and 1M grid resistors; 5.1k grid stoppers and 1k / 5 W screen resistors; 100k negative feedback with a 5k presence pot, 4.7k and 0.1 uF; supply 10k / 2 W with two 50 uF capacitors (0.5 s, as used) at about 424 V. No code change.

## Circuit-derived levels (v83)

What the "Derived" menu uses, and how sure I am of each number.

- **Power section.** Closed-loop response H = A Zl/(Zl+Zo) / (1 + beta A Zl/(Zl+Zo)), with the open-loop gain A (low corner, high-frequency poles), output impedance Zo (tube plus transformer leakage), a speaker model (voice-coil resistance and inductance plus a mechanical resonance, about 95 Hz, peak five times nominal) as the load Zl, and the feedback fraction beta from the feedback resistor, the presence network (resistor, pot, 0.1 uF) and the cathode resistor. Solved for five families and stored as four RBJ filters (peak 100 Hz, peak 1.2 kHz, peak 3.5 kHz, high shelf 7 kHz) by feel (loose, normal, tight) and five presence steps. Families: American combo (820 ohm feedback, read from the 1972 schematic), British 50 W (100k feedback), British lead (39k feedback, read from the SLO-100 schematic: about 2.6 times the 50 W), light feedback (220k), no feedback. The 50 W amp's 100k feedback resistor is confirmed by a second schematic (see below). The light-feedback family's resistor and the speaker model's resonance and Q are estimates; the fit error is under 0.3 dB RMS.
- **Which amp uses which family.** Clean: American combo. Chime Thirty, Tweedle Dee and Citrus Doom: no feedback. Brown Eye, Club Crunch, Master Cranker: British 50 W. Slow Burn, Modern Maul, 5555 Block, Revved Up: British lead (the 6L6 power board of the 5150 II schematic has 2.2k grid stoppers and 100 ohm screen resistors; I did not find its feedback resistor with confidence, so this family is its closest match, not a reading). Bumble Lead and Marque Dream: light feedback.
- **Supply.** Fast droop depth = 5 Ipk Rs / B+ (per amp, times twice the Sag knob), slow droop 0.45 of that; time constants: fast 0.02 s scaled by Rs, slow = dropping resistor times filter capacitor where read (American combo 2.2k x 20 uF; chime combo 22k x 32 uF; 50 W British 10k x 50 uF; 100 W lead 15k x 10 uF). B+, Ipk and Rs are estimates from the tubes and transformers (480 V / 0.45 A / 60 ohm for the combo, 330 V / 0.22 A / 150 ohm for the chime combo, 420 V / 0.30 A / 100 ohm for the 50 W, 495 V / 0.40 A / 90 ohm for the 100 W lead, 480 V / 0.50 A / 80 ohm for the 120 W modern amp). The 5150's dropping network was not read, so its slow time constant (0.2 s) is a guess. The transformer saturation amounts (0.15 to 0.40) are also guesses, larger for smaller transformers.
- **Voicing.** Cathode bypass corner = geometric mean of the zero 1/(2 pi Rk Ck) and the pole with Rk in parallel with ri/(mu+1); coupling corner = 1/(2 pi R C). American combo (1.5k / 25 uF cathodes, 0.02 uF into 1M, 120 pF bright cap on a 1M pot) and the 50 W British amp (2.7k / 0.68 uF first stage, 0.022 uF into 470k) only. Values for the other amps were not read with enough confidence to use.
- **Levels.** Each level is trimmed per model so the guitar test signal (gain 0.5) has the same RMS as classic (measured: the same to 0.1 dB). Other settings differ by a dB or two.
- **Measured (guitar test signal, model 2, feel normal, presence 0.5).** Power section minus classic, small signal: 100 Hz +2.6 dB, 400 Hz -0.1, 1.6 kHz +1.2, 6.4 kHz -2.3 after the level trim. At presence 1.0 the classic shelf is 5.5 dB brighter at 6.4 kHz than the derived response.

## Amp mode (v87)

Mode merges the Circuit Amp, Stage model and Derived settings. Circuit-derived = stage model curves plus the full derived level (power section, supply, feedback loop, component voicing); nothing in it is new modelling. Timing on the build machine for 8 s of audio at 48 kHz, 256-sample blocks, Slow Burn / Club Crunch / Modern Maul: Classic 0.12 to 0.16 s, Circuit-derived 0.17 to 0.20 s, Full circuit 3.5 to 3.6 s. Reported latency was 3 samples in all three; the Full circuit mode is therefore expensive in CPU but not in reported latency. These are single-machine figures, not benchmarks.

## Mesa Boogie schematics (v89)

Read from three uploaded PDFs: a Mark IIC+ preamp and power amp (one page is a low-resolution scan; values marked "read" were legible, a few labels were ambiguous), a Mark IV preamp (clean, legible), and a Dual Rectifier (revision RF-1F preamp, power amp, effects loop and supplies, clean). Applied only to the two amps they correspond to in this plugin, Modern Maul (Dual Rectifier style) and Marque Dream (Mark style), and only in Circuit-derived mode.

**Dual Rectifier, read.** Preamp: V1A 220k plate, 1.8k / 1 uF cathode (plus 47k switched in), 1M grid leak; V2A 100k plate, 1.8k / 1 uF, 20 pF on the grid; V2B 100k plate with a 39k cathode resistor; V3A 220k plate, 1.8k / 1 uF, 220k grid stopper; V3B cathode follower (415 V plate, 100k load) into the stack. Couplings 0.02 uF. Stack (already used in v84): 250k treble, 1M bass, 25k mid (26k on the print), 47k slope, 0.02 uF caps, 680 pF (red) and 600 pF (orange) treble caps. Power amp: 12AX7 long-tail inverter with 82k / 90k plate loads and 120 pF across them, 47k coupling to four power tubes through 1.5k grid stoppers, 1k / 2 W screens, bias -51 V (6L6) or -39 V (EL34) selected by a switch. Feedback: from the 8-16 ohm tap through 0.1 uF and a 47k resistor (a second 47k can be switched in parallel), into a node with 4.7k to ground in parallel with a 26k presence pot behind 0.1 uF, plus 10k from the preamp: a feedback fraction of roughly 0.06 with one 47k (about -24 dB) and about 0.12 with both. That is the same ballpark as my British-lead feedback family (39k), so I did not add a new family. Supplies: a 5U4 tube rectifier pair or four silicon diodes (selectable), 2 x 220 uF / 300 V in series as reservoir with 150k balancing resistors, 460 V at the reservoir, a choke, 454 V (30 uF), 2.7k to 422 V (30 uF), 22k to 406 V and 402 V (30 uF each). Applied: first-stage cathode corner (1.8k / 1 uF) as the first-stage tightness; supply with 460 V, a 0.081 s slow time constant (2.7k x 30 uF), 80 ohm source resistance (my estimate, between the silicon and tube rectifier) and the previous peak current and transformer saturation (estimates). Not applied: the 39k cathode on V2B (a different stage type than my curves), the second and third stage cathode corners (a partial shelf, which my single high-pass per stage cannot represent), the effects loop.

**Mark IIC+, read.** Rhythm: V1A 180k plate, 1.6k cathode with 0.47 uF (and 2.2 uF with 22k on the bass-shift switch), 1M grid leak; V1B 100k plate, 1.6k / 16 uF; 0.047 uF coupling; volume pot 1M with a 100 to 120 pF bright cap (pull switch); stack 250k treble, 250k bass, 10k mid, 100k slope, 250 pF / 0.1 uF / 0.047 uF, plus a 750 pF treble-shift cap and a 10M resistor. Those are the same values as my American combo passive stack, so no new stack. Lead: V4A and V3B with 82k and 170k plates, 3.1k and 1.5k cathodes, a 1M lead-drive pot. Power amp: a 12AX7 driver and long-tail inverter (82k plate loads), four power tubes (6L6GC, or 6CA7 / EL34 on the print), 2.2k grid stoppers, 470 ohm / 2 W screens, a Simul-Class switch that biases one pair into class A. Feedback through a 250k presence pot, 330 ohm, 56k with 0.005 uF across it, into the inverter cathode network (3.3k with 0.047 uF and 1.5k): partly legible, so only noted, not applied. Supply: silicon bridge, 2 x 220 uF reservoir (a pair of 220 uF per rail in Simul-Class), 480 V, choke, 413 V (30 uF), 5.6k to 397 V (30 uF), 1k to 392 V (30 uF). Applied to Marque Dream: first-stage cathode corner (1.6k / 0.47 uF, about 300 Hz by my geometric-mean rule, so noticeably tighter than the earlier by-ear low end; compare against the real amp's bass-shift switch, which I did not model), a 120 pF / 1M bright cap, and a supply of 480 V, 0.17 s (5.6k x 30 uF), 60 ohm source resistance, peak current 0.35 A and transformer saturation 0.20 (the last two are my estimates). Not applied: the graphic EQ (five-band, transistor), the lead channel, the reverb.

**Mark IV, read.** Rhythm channels match the IIC+ stage values (180k and 100k plates, 1.6k cathodes, 0.47 uF and 16 uF cathode caps, the same stack values, 120 pF bright cap). Lead: V3A 82k plate, 1.6k / 2.2 uF; V3B 270k plate with 600 pF, 3.3k / 0.22 uF cathode (22k switched in), 0.047 uF output coupling; lead-drive pot 1M. Reverb driver and recovery stages 12AT7. Recorded only: I have no separate Mark IV model, and the lead channel does not map to one of the three stages in my chain.

**Levels.** The new component values moved the Marque Dream level by about +1.6 dB relative to the previous derived mode, so its component-voicing trim is -1.6 dB; Modern Maul moved about -0.2 dB (trim +0.2). After that both match the v88 Circuit-derived level on the guitar-like test signal (within 0.1 dB on the three-gain average). Classic mode is unchanged (48 / 48 compatibility renders bit-identical to v61). I did not listen to either amp.

## Mark V schematic (v90)

Read from an 8-page vector PDF (power supply, preamp parts 1 to 3, reverb, EQ, loop and driver, power amp). Clean and legible; I read the supply, channel 1 and 2 preamp, and the driver and feedback pages in detail, and only glanced at the rest.

- **Supply (read).** 4 x 1N4007 or a tube rectifier (selectable), 2 x 220 uF / 300 V in series as reservoir with 220k balancing resistors, 448 V at the reservoir (90 W setting; the print gives offsets for 50 W and 10 W), a choke, 445 V (30 uF), 5.6k / 2 W to 376 to 410 V (33 uF), 1k from there onward. This agrees with the Mark IIC+ (5.6k into 30 uF), so Marque Dream's supply now uses 448 V and a 5.6k x 33 uF = 0.185 s slow time constant.
- **Feedback (read).** The speaker feedback goes through a channel-selected presence control (separate pots per channel, with 0.1 uF, 0.001 uF and 500 pF caps), then 3.3k in parallel with 500 pF, then 56k in parallel with 0.005 uF, into the inverter's cathode network (3.3k with 0.047 uF, then 1.5k to ground). That is the same network I partly read on the Mark IIC+, so that reading is now confirmed. A feedback fraction of about 0.075 (roughly -22 dB, before the presence pot) is the same ballpark as the dual-rectifier amp, so Marque Dream moved from the light-feedback family to the British-lead family (loop gain 1.8 / 3.7 / 6.1 for loose / normal / tight). This is a judgement from the resistor values, not a solved response; the open-loop gain is still my estimate.
- **Preamp.** Channel 1 stack: 250k treble, 250k bass, 10k mid, 100k slope, 250 pF / 0.1 uF / 0.047 uF (same as before), 120 pF bright cap on the 1M gain pot. V1A cathode 1.5k plus 1.5k switched in, with 0.47 uF and 15 uF switched in by relays; V2A 1.5k cathodes with 0.22 uF and 15 uF. Not applied beyond what v89 used (1.6k / 0.47 uF first stage), because the Mark V's relays change the cathode network per mode.
- **Driver.** 12AX7 long-tail inverter, 82k and 91k plate loads, 22k / 470 ohm / 100k / 150k network, 250 pF and 75 pF across the plates, relay-switched.
- **Levels.** Moving Marque Dream's feedback family lowered its level by about 1 dB, so its power-section trim went from -1.50 dB to -0.50 dB; levels again match the v88 level within 0.1 dB on the three-gain average.
- **Not applied:** the five-band graphic EQ, the reverb, the effects loop, the per-channel relay logic, the power amp's simul-class bias network.

## Tweed, SLO-100 and 1959 schematics (v93)

**Fender 5E3 (built into Tweedle Dee).** Read: V1A 12AY7 with 100k plate and 820 ohm / 25 uF cathode, 0.1 uF to a 1M volume pot; 1M tone pot with 500 pF at its top and 4.7 nF at its bottom, wiper to the volume wiper; V2A 12AX7 100k plate, 1.5k / 25 uF, 0.022 uF coupling; V2B cathodyne inverter (56k plate, 1.5k + 56k cathode); two 6V6 with 1.5k stoppers, 220k grid leaks, 470 ohm / 1 W screens, shared 250 ohm / 25 uF cathode; 5Y3 rectifier, 345 V plates, 320 V screens through 4.7k x 16 uF, 224 V preamp through 22k. Used: the tone network, cathode corners, supply. Estimates: the 12AY7's plate resistance (25k), the volume pot position (fixed), Miller capacitance (100 pF), rectifier resistance (250 ohm), peak current (0.12 A). The tone network is exactly solved at each Treble setting; measured at Tone 0 / 0.5 / 1: response at 6.4 kHz is about -26 / +2 / +11 dB relative to the middle-band average.

**Soldano SLO-100 (more applied).** Read from the preamp and power amp drawings: V1b 220k plate, 1.8k / 1 uF, 0.02 uF coupling with 2 nF across 470k, 500k OD volume, 470k to V2a; V2a 100k plate, 1.8k / 1 uF; V2b cold-clipper stage (100k plate, 39k cathode); V3b 220k, 1.8k / 1 uF; V4b 220k, 2.2k; cathode-follower outputs; late tone stack 250k treble / 1M bass / 25k mid, 47k slope, 470 pF and two 0.02 uF; OD and Normal master volumes (1M); V5 long-tail inverter (82k / 100k plates, 47 pF); 6L6GC with 2.2k stoppers, 220k grid leaks, 475 ohm screens; 39k feedback from the 4 ohm tap with presence 25k + 0.1 uF and 4.7k; supply 497 V plates and screens, 378 V (10k, 40 uF) and 359 / 350 V (15k, 10 uF). Applied: cathode corners and supply time constant. Not modelled: the cold clipper, the opto switching, the FX loop.

**Marshall 1959 (1970).** Read from a low-resolution drawing: V1 both halves 100k plates with 68k stoppers (68k, 1M); channel 1 cathode 820 ohm / 250 uF, channel 2 2.7k / 0.68 uF; 0.022 uF and 0.0022 uF couplings; 470k mixers; V2A 820 ohm (1k) / 0.68 uF; V2B cathode follower (100k); stack 250k / 1M / 25k with 500 pF, 0.022 uF, 0.022 uF and 33k; V3 inverter (82k / 100k, 1M, 470 ohm, 47 pF); four EL34 with 5.6k stoppers and 1k screens; fixed bias from a diode and 27k (22k); bridge rectifier, 20k / 10k dropping stages with 50 uF capacitors. The feedback network could not be read with confidence, so the existing family is kept.

## Five amps added in v95
Parameter values 16 to 20, three voicings each plus a fourth, in the amp menus under Low / mid gain (Harmonic Haze) and High gain (the rest). All voicings are hand-tuned from the sources above; **none has been listened to**. Measured: levels at every Gain setting (Auto Level table extended), no non-finite output in Classic or Circuit-derived mode, and the spectrum of a test riff to confirm that each lands in the range of the existing high-gain amps. Circuit-derived mode for these amps uses: the stage and power curves of the nearest existing type, a power-section family and supply estimate (see CIRCUITS.md for which numbers were read and which are guesses), and for Harmonic Haze the first three cathode corners read from the schematic.

## v0.1: Hi-Wat Headroom and Console DI (amps 21 and 22)
- **Hi-Wat Headroom** is modelled on the Hiwatt DR103 (owner's manual and schematic received). Read from the schematic: the tone-stack values (1000 pF, 220 K, 220 pF, 220 K treble pot, 22 K, 100 K mid, 470 K bass, 100 K / 47 nF / 47 nF), which I solved with a source resistance of about 40 K (an estimate). That gives a scoop near 250 Hz at noon and a rising top, which set the Normal and Brilliant EQ. Power supply and headroom numbers are in PowerSection.h. Voicings Normal and Brilliant follow the two inputs, Linked jumps them, Open is my own variation. Estimates: stage gains, trims, the 40 K source.
- **Console DI** is modelled on the Neve 1073 (manual received): 110 Hz low shelf, 12 kHz high shelf, class-A transformer-coupled colour. Gains and saturation amounts are my estimates; the manual gives specs, not a circuit I could solve here.
- Received but not built: an SSL 82E01-era channel amplifier card (op-amp mic and line stages, no EQ) and the API channel strip manual (512C preamp, 550A EQ, RMS compressor with a Thrust filter). The API EQ and Thrust compressor are the interesting parts, a possible later voicing and compressor mode.
- Levels measured only. Nothing listened to.

## v0.2: Marmalade (amp 25)
- **Marmalade** is an Orange Rockerverb-style 50 W head (Crunch and Heavy are its dirty channel, Clean its clean channel, Sharp a derived brighter voicing). Preamp values were read from the Rockerverb preamp, Dual Terror and Dark Terror sheets and the Jim Root Terror DC-voltage sheet (V1A 1.5k / 10 uF cathode, 1 nF into 220k + 220k, 2.2 nF into about 700k, a 100 pF bright cap on the 1M gain pot, a Marshall-type stack with a 39k slope). Tone and distortion were then fitted against plain captures of a Rockerverb 100 MkIII and a Rockerverb MkIII (Hybrid mode, measured: octave-band shape within a few dB, 196 Hz harmonic structure close). The coupling cap into the second stage is set to 1.5 nF instead of the read 1 nF because it fits the captures better. The Fortin TS808-boosted and TI Boost captures and the Jim Root Terror captures were not used for the fit.
- Auto Level measured (flat within 0.5 dB across Gain, all four voicings). Trim values and the Classic and Circuit modes were only checked for level, not tuned. Nothing listened to.

## v0.2: Plex Appeal and Matt's Amp (amps 23 and 24)
- **Plex Appeal** is a Plexi-style 100 W head. Stack solved as a standard Marshall-type network (250k treble, 1M bass, 25k mid, 33k slope, 470 pF / 22 nF / 22 nF) with a cathode follower of about 1k as the source. The mod voicing (Modded) is built from the Jose-style mod list on ampmodlab.com (V0 / cascade stage, bright cap reduced, a pre-tone-stack master volume, a diode clipper, a softer feedback network), with the mod's component values read from screenshots of the page you sent. Only the effects of the mods are modelled (tighter, more gain, less fizz); I did not rebuild the circuit stage by stage. The Stock voicing is close to the existing Club Crunch Crunch voicing and I made it brighter to separate them.
- **Matt's Amp** takes its bass and treble network from three Orange schematics (an early-1970s OR120 Graphic MkII, a GRO 100 measured in 1975 and a GT120). The three networks share one topology and differ in values: 100k into a 1M bass pot with two caps and a leg resistor, a small cap into a 1M treble pot with a cap to ground, joined through 100k. I solved all three and used the solved shapes as the starting point for the EQ rows; the gain, clipping and sag values are my estimates by analogy (Club Crunch and Citrus Doom rows), and the voltages are not printed on the sheets. The fixed "FAC" caps of the early amps are modelled as a top cut (Graphic) and a bass cut at the input (Heavy). The three sheets disagree on several parts, so each voicing follows its own sheet and the fourth (Mix) is my own mix.
- Nothing listened to. Levels measured and flat across Gain with Auto Level on.

## Dual Thirty (amp 28)
- Matchless DC30-style two-channel 4xEL84 amp. Brand names appear only in this file, CIRCUITS.md and AMP_SOURCES.md.
- Estimates: first-stage cathode value, tone-stack interaction (folded into the voicing's EQ offsets), Pushed/Cut settings. Not auditioned against a real amp.

## Solid Metal (amp 29)
- Hybrid and JFET solid-state metal amp. Brand names appear only in this file, CIRCUITS.md and AMP_SOURCES.md.
- Estimated: clipping asymmetry, stage gains, tone offsets. Compared by gain structure only against a Big Bottom-style NAM capture. Not auditioned.

## v97 and later: Eclipse and Marque Five

| Plugin name | Inspired by | Notes |
|---|---|---|
| Eclipse | Meshuggah Blackout (Fortin) | Hi/Lo input and pulled/pushed master volume from the manual; fitted loosely to the Salvation Audio captures. Not listened to. |
| Marque Five | Mesa Boogie Mark V (three channels) | Clean, Crunch, Lead and Extreme fitted to the Mark V NAM captures (Ch 1 / Ch 2 / Ch 3 MK-IIC+ / Ch 3 Ext and MK-IV); first-stage and coupling values from the Mark V preamp schematic. Not listened to. |

## Cedar Twelve (amp 30)

- **Cedar Twelve:** small blackface-style combo. Preamp and tone-stack idea from the Twin-style amp; two 6V6 power stage, about 22 W, silicon rectifier, small transformer. Supply numbers (B+ 420 V, peak current and source resistance) are estimates, not read from a schematic. Big Spring is a 40 W two-6L6 combo with a 15-inch speaker, again by ear and by what the amp is known for. The only reference is the Twin Sparkle voicings (compression and tone measured against them).

## Triple Blend (amp 31, b5)

Three engines run on the same input and are summed before the cab: Amp A's own engine (the Cedar Twelve Big Spring voicing), plus two parallel engines (Bumble Lead Rock voicing; Hi-Wat Headroom Normal voicing). All three use Amp A's tone knobs, Presence, Gain (with small per-voicing offsets), Power Drive and Resonance. The part levels, per-voicing balances and gain offsets are my placeholders (kBlendLevelOff / kBlendGainOff), not measurements of any rig.

## b14 and b21 amp notes

- b14: Jazz Box is my reading of the big, dark, solid-state jazz amps (the Polytone-style idea you asked about); Brit Eighteen is my reading of the small 18 W British EL84 combos. Both tuned from general reputation only, no schematic or capture.

- b21: Diesel Fuel fitted to Diezel VH4 captures you supplied; Cutaway harmonic balance partly fitted to more Prometheus captures.

# Gain: fuzz, overdrive and boost

## Boost saturation modes

- Boost saturation modes: "EP" is voiced after the small tube preamp inside a classic tape-echo unit, which players use as a warm boost; "Console" is voiced after the transformer-coupled preamp of a classic British recording console (the Neve-type sound you mentioned); "Treble" is a treble booster in the tradition of the 1960s British treble-boost pedals; "Tube" is a generic triode stage. The "Mid hump" tighten style (named for what it does) takes its low cut and mid hump from the Tube Screamer-type overdrive pedal that metal players use as a tightener (it was called "Screamer" in my drafts; I renamed it to keep brand words out of the plugin); "Focus" and "Low cut" are generic.

## Fuzz and overdrive pedals

| Plugin name | Inspired by | Provenance |
|---|---|---|
| Fuzz: Wall (sustained) | Big Muff | You asked |
| Fuzz: Tight (op-amp) | ProCo Rat | You asked |
| Fuzz: Gated (velcro) (greyed out in the menu, kept for old sessions) | Fuzz Factory | You asked |
| Fuzz: Buzzsaw | Boss HM-2 | Circuit-derived (v93) from the schematic you sent. Read: all resistor/capacitor values of the tone network (two 10k pots, three series-resonant branches), the stage gains, the clipper. Estimated: transistor and op-amp behaviour, the diode types, the exact stage gains (I computed them). Not listened to. Modded-HM-2 options (v94, N.E.W. DIY HM-2 schematic): split Mid / High pots, Soft and Hard clipper switches, Blend to a clean copy with Invert. |
| Fuzz: Face (round, germanium or silicon) | Dallas Arbiter Fuzz Face (germanium and silicon versions) | You asked |
| Fuzz: Bender (raspy, 2 or 3 stage) | Sola Sound / Vox Tone Bender (Early = MkI-style, Classic = MkII-style, Late = MkIII-style) | You asked |
| Fuzz: Factory (gated, squealing) | Fuzz Factory (a fuller take than "Gated"; with a germanium / silicon switch) | You asked |
| Fuzz Wall versions | Smooth = Ram's Head-style Big Muff, Gritty = Triangle-style, Mid-forward = Green Russian-style, Heavy = Black Russian-style; the Op-Amp mode is an op-amp clipping variant | You asked |
| Fuzz options: Starve, Guitar Vol, Buffer, Drift, Hiss, Octave Up | Behaviours of germanium fuzz pedals (low battery, volume-knob cleanup, pickup loading, temperature drift, octave-up rectifier fuzzes) | You asked |
| Overdrive: Transparent | Klon-style overdrive, in the style of Little Cubes LC_VODA | You asked |
| Overdrive: Open | Fulltone OCD-style overdrive | You asked ("Fulltone OCD style overdrive") |
| Overdrive: Tight (Attack, Gate) | Horizon Devices Precision Drive (Misha Mansoor) | My reading |
| Overdrive: Screamer (Silicon / LED / Germanium / Asymmetric clipping, Classic / Mild / Open voicing, Bass, Mid, Mid Freq) | Ibanez Tube Screamer family (TS808 / TS9 / SD-1-style voicings) and the EarthQuaker Devices Palisades (Bass, Mid and Mid Freq controls, clipping choices) | You asked ("versatile TS style overdrive (think EQD Palisades)") |
| Overdrive: Distortion (Silicon / LED / Germanium / Asymmetric clipping, Scoop / Flat / Boost Mids) | Boss DS-1 (the stock mid dip, plus the common LED, germanium and asymmetric diode mods) | You asked ("a DS1 style distortion") |

### Overdrive styles (added)

| Style | Inspiration |
|---|---|
| Tube Driver | A B.K. Butler Tube Driver-style tube overdrive (used by Eric Johnson as a lead boost): full-range, warm, creamy, mid-forward when pushed. A hand-built approximation, not a circuit model |

### Circuit modes (v77)

| Part | Source |
|---|---|
| Screamer, Circuit mode | The TS808 analysis at ElectroSmash (tube-screamer-analysis): non-inverting op-amp stage, 4.7k + 47 nF to ground (720 Hz), 51k + 500k drive pot with 51 pF across it, 1N4148 pair in the feedback loop, 1k/0.22 uF tone stage with the 20k tone pot. Drive law: stock 51k to 551k on a log pot; below that, an extension down to 1.5k so the range is not narrowed. Bass moves the 720 Hz corner (about 70 Hz to 1.8 kHz). |
| Tight, Circuit mode | The Collision Drive building document (PCB Guitar Mania), a Horizon Devices Precision Drive-style metal overdrive: JRC4558 stage, 1k + attack capacitor (470n, 330n, 220n, 100n, 68n, 47n), 10k + 500k drive pot, 47 pF, 3k9/5k bright stage, 1N4148 diodes (LED, germanium, asymmetric and no-diode options added). The schematic's C3 reads as nF in one place and pF in another; I used 47 pF. An unlabeled resistor was ignored. |
| Wall, Circuit version | The Big Muff Pi analysis at ElectroSmash (V3 schematic): input booster, two diode-limited stages (470k with the 1 uF series capacitors), the passive tone network (solved exactly, trapezoidal capacitors) and the output stage. Silicon, LED and germanium diode choices are added. The Big Muff Page link sent later (part 1, 1969 to 1974) has its schematics as images; its text only gives variant notes (for example 33k or 22k tone resistors, 390k feedback in some early V1 circuits), so those are not modeled yet. |

| Wall, variants B and C | The later Russian-built Big Muff circuits (the Civil War / tall font 7B and 7C with 1 nF + 1 nF in series = 500 pF feedback capacitors, and the bubble font 7C and small-box Black Russian (version 8) with 470 pF), from Kit Rae's traced schematics on bigmuffpage.com (sent by the user as images). Values used: 39k input, 470k feedback, 12k collectors, 390 ohm emitters, 0.1 uF couplings, 0.047 uF in series with the 1N914 pairs, 22k and 20k tone resistors, 0.0039 uF and 0.01 uF tone capacitors, 10k and 2.7k output stage. The occasional 0.0068 uF C9 is not modeled. |
| Wall, op-amp build | The Dream Fuzz (PedalPCB) schematic and parts list: RC4558 booster and Sallen-Key stage, UA741 clipper with three 1N4148 each way, Muff-style tone network with tone bypass. The 1 uF capacitor in the tone network's low-pass branch (about 28 Hz) is ignored. |

Method for variants B and C: the first circuit's published gains (16.7, 23.4 and 25.2 dB) and poles (1.2, 1.78 and 1.17 kHz) were used to calibrate a small-signal transistor model (one gain and one effective-capacitance factor per stage); the same factors were then applied to the changed resistor and capacitor values.

Schematics received but not yet modeled (amp circuits, planned as a separate step): the 5150-style full amp schematic (5 pages, including the tone stack and phase inverter) and the 1980s 50 W / 100 W lead-series amp preamp and power circuit diagrams (standard and master volume).

Assumptions: the guitar signal is taken as 1.0 = 1 V, the op-amp is a gain-bandwidth-limited ideal amplifier, and the diodes follow the Shockley equation with typical saturation currents.

### Fuzz styles (added)

| Style | Inspiration |
|---|---|
| Nuke (wave-shaper) | The idea behind 3 Sigma Audio's Tactical Nuke: a wave-table waveshaper (a short table used as the transfer curve, blendable), with bit and sample-rate reduction and a resonant output filter. Five procedural curves here, not its 512 tables; my own design |
| Crusher (bits and rate) | A generic bit-crusher / sample-rate reducer with drive and an output filter, in the spirit of the Tactical Nuke's bit and sample-reduction sections |

## Classic versus Circuit modes of the pedals (measured, v82)

Small-signal response of the Circuit mode minus the Classic mode, same knob settings, with a clean amp behind both (negative = the Circuit mode is quieter there). The Circuit modes cost about the same CPU as the Classic ones (0.52 s against 0.48 s for 8 s of audio).

| Pedal and settings | 100 Hz | 200 Hz | 400 Hz | 800 Hz | 1.6 kHz | 3.2 kHz | 6.4 kHz |
|---|---|---|---|---|---|---|---|
| Screamer, gain 0.2, tone 0.5 | +6.1 | +7.3 | +2.0 | -0.6 | -1.9 | -6.9 | -14.4 |
| Screamer, gain 0.5, tone 0.5 | +14.2 | +2.7 | -0.9 | -3.2 | -3.4 | -8.3 | -16.0 |
| Screamer, gain 0.8, tone 0.5 | +9.6 | +2.3 | -0.5 | -3.0 | -2.9 | -7.7 | -15.6 |
| Tight (precision drive), gain 0.5 | +3.9 | +1.8 | +4.0 | +5.6 | +14.4 | +17.3 | +16.1 |

What this says: the Circuit Screamer keeps its low end (the clean path passes it) and rolls off the top much earlier, as the real tone control does; the Circuit Tight has far more high-frequency gain than the Classic one (its 4558 stage is wide open above the corner) and distorts more at low levels. The Classic modes were tuned by ear and are left alone so saved sessions sound the same.

## Pedal analyses (v91)
Read from the ElectroSmash analysis pages (twelve pedals) and applied only where the numbers were stated.

| Pedal | Used in | What was taken |
|---|---|---|
| Boss DS-1 | OD Style Distortion, Circuit Mode (new) | Booster gain 56 and 33 Hz corner, op-amp gain 1 + 100k/4.7k, 0.47 uF in the 4.7k leg, 100 pF across the pot, 2.2k / 10 nF into 1N4148 clipping, tone network 6.8k / 0.1 uF, 22 nF / 6.8k, 20k pot |
| Vox V847 / CryBaby GCB-95 | Wah Voice: Classic circuit (new) | Sweep 450 Hz to 1.6 kHz (the page's figures; which end is heel and toe is not stated) |
| ProCo Rat | Fuzz: Tight | Already matched: gain 1 + R/43.4 ohm, tone 475 Hz to 32 kHz; nothing changed |
| Fuzz Face, Rangemaster, Klon Centaur, Guv'nor, MXR Distortion+, Dyna Comp, MicroAmp, Phase 90 | none | Read, not applied. Fuzz Face: the existing Face already uses a similar input load and gain range, and the page's values (hFE 70 to 130, Zin 5 to 8 kohm) fit inside it. Phase 90: stage values given (24k, 47 nF) but not the JFET resistance range, so the sweep cannot be set from the page. Dyna Comp: no ratio, release or detector equations. Rangemaster, MicroAmp, Distortion+, Guv'nor, Klon: no matching pedal model to attach them to (the existing Transparent and Distortion styles are hand-tuned) and no clean way to add one without a new style, left for a later version |

Not modelled in the Distortion circuit: the booster's own treble roll-off (the page gives no figure), the input and output buffers, and the tone network loading the clipper node. The op-amp bandwidth is assumed 1 MHz because the parts list names several op-amps. The wah's 18 dB peak and its Q are not applied: the page gives no Q, and the heel/toe mapping is ambiguous.

## Pedals in the Drive, Boost and Comp sections (v93)
| Where | Pedal it follows | Read from the analysis | My own estimate |
|---|---|---|---|
| Drive: Transparent (Grip = Edge) | Klon Centaur | Gain-pot law, clean and gain paths, treble shelf, germanium clipping network, buffer; the LC_VODA analysis is still to come and will replace the estimates | Input network (1k, 0.33 uF), minimum feedback 1.5k, 100k gain pot, 390 pF, 3 MHz op-amp bandwidth, clipper 1k / 100 pF, 106 Hz feed-forward corner, the 408 Hz shelf (+18 / -8 dB), the blend weights (drive = k, feed-forward 0.3, clean = 1 - 0.6k) |
| Drive: Op-Amp Distortion | MXR Distortion+ | Gain 1 + 1M / (4.7k + pot), 47 nF, 10k / 1 nF into germanium diodes (the Clipping menu overrides) | 1 MHz bandwidth, 3.5 V swing |
| Drive: Stack Drive | Marshall Guv'nor | Gain 1 + 100k / 2.2k, 100 nF, 120 pF, x68 second stage, red LEDs, 15.3 kHz output low-pass | The whole tone section (the page lists no values), the LED node values |
| Drive: Blues Driver (v0.2) | Boss BD-2 Blues Driver | Stage 1: 1k5 / 150 nF shunt, 22k + 250k Gain pot, 47 pF, 2 MHz; fixed stage 2 (2k2 / 1 uF, 33k, 100 pF); silicon diode clipping between the stages; about +6 dB bump near 120 Hz; Tone low-pass 1 to 12 kHz; 4.5 V swing | Deliberately not matched to the published curves where that cost range: the full 250k pot (the published sweep tops out nearer 80k, so it is hotter at max Gain), the wider Tone range (the published sweep stops near 4 kHz), 2 MHz instead of 1 MHz. Also estimated: inter-stage divider (fitted to the published +7 dB at minimum Gain), diode node, 120 Hz bump, output trim |
| Boost: Clean | MXR MicroAmp | Gain 1 + 56k / (R5 + 2.7k) times 0.955, 4.7 uF, 47 pF, 4.5 V rails; R6 = 2.7k is back-calculated from the page's own gain table | Reverse-log pot law |
| Boost: Treble | Dallas Rangemaster | 5 nF into about 11k (2.9 kHz), gain 80 at full pot, 10 nF output coupling, 16 Hz | The +7 V / -1.8 V limits; the 5 nF to 47 nF Sat range |
| Comp: Dyna (OTA) | MXR Dyna Comp | Input and output high-passes, the high-frequency lift (about 1 dB at 300 Hz to 4 dB at 10 kHz) | Everything about the detector: the page gives no ratio, attack or release; the output trim (+8 dB) |

Every Drive style except Open and Tube Driver, and every amp, now runs from component values only (the "circuit-derived" path). That path is the one to retune from readings: each estimate above is one number in CircuitModels.h. Levels were matched by measurement only; none of these has been listened to against the real pedals. In v92 these pedals were extra Style entries (6 Treble Boost, 7 Clean Boost, 8 Op-Amp Distortion, 9 Stack Drive, 10 Sustain Comp); in v93 the Style menu ends at 7. A v92 session that used 6 or 7 now selects Op-Amp Distortion or Stack Drive, and 8 to 10 are clamped to Stack Drive; re-pick the style (and use the Boost and Comp sections for the other three).

## Pedals added in v96
| Style | Based on | Source |
|---|---|---|
| Power Chip | EQD Acapulco Gold | Guitar Mania "Acapulco Tone" build docs |
| Doom | Life Pedal (Life Device SMD) | build docs |
| Wall, Circuit (modern build) | Hizumitas | Elk Device build docs |

## v97: Metal Pedal fuzz style
Merged approximation of the Lichtlaerm Gehenna and Total Distortion Worship, from their published control lists and descriptions (no schematics). Medusa and Altar were not built as separate styles: Buzzsaw (HM-2 circuit) and Wall (Muff circuit) already cover their cores. Ragnarøk (tube preamp pedal), Nostalgia (chorus/vibrato) and As Above So Below (octave reverb) are still to do.

## v97: Distortion + Reverb fuzz style, and what was not built

- **Distortion + Reverb** fuzz style: inspired by the Walrus Audio Melee Wall of Noise (distortion and reverb in one box, with an order switch). Approximation from the product description; no schematic. The reverb is an 8-line feedback delay network; the octave-down program and the reversed swell are my own reading of "wall of noise" and are not matched to the pedal.

- **Not built, and why**: Lichtlaerm Medusa (HM-2 style plus EQ and gate: covered by Buzzsaw plus the Metal Pedal EQ and Gate), Altar (Muff style plus boost: covered by Wall plus Boost), Ragnarok (tube preamp pedal: covered by the amp models). Archetype-style, Dumble-style, Two-Rock-style and blackface-style reverb amps: the existing clean and edge-of-breakup voicings are within the same range.

## b20: Fuzz Factory and Open drive re-fit

- b20: Fuzz: Factory style re-fitted to 15 Fuzz Factory (Vexter) captures you supplied. Overdrive: Open has a Circuit Mode built from a Fulltone OCD schematic you supplied.

# Compressor

## Compressor

| Plugin name | Inspired by | Provenance |
|---|---|---|
| Compressor | Empirical Labs Distressor-style (the Dist modes and the saturation options) | My reading |
| Compressor: Bloom, Smack, Clarity, Transformer | Abasi Micro-Aggressor | You asked |

## Thump style (b31)
Bloom, Smack and Clarity (a deeper, blooming main compressor, a fast parallel compressor with lifted lows, and a tilt EQ) were a feature of the Abasi Micro-Aggressor idea (you asked); in b31 they became the Thump style. The "10:1 Soft" ratio was removed in b32; the ratio menu now has eight entries and applies to every style except Dyna. Own design; nothing matched to a unit.

# Mod FX, wah, phaser, chorus, filters and lo-fi

## Lo-Fi warble

**Lo-Fi Warble pedal.** You asked for a "Zvex lofi junky type pedal". My reading of that pedal is a worn, wobbly, lo-fi analog vibrato / chorus with a crushed, noisy character. I did not copy its circuit or controls (I do not know them in detail); the plugin's pedal is my own design built on that idea: a pitch warble (rate, depth, shape), random drift and flutter, a dark tone control, saturation plus bit/sample-rate crushing ("Dirt"), hiss and crackle, random dropouts, and dry/wet mix. It is generic in the UI ("Lo-Fi Warble"). Treat the match to the real pedal as a guess.

## Mod FX notes

- Mod FX: Flanger and Through-zero flanger (the tape-flanging idea and the through-zero analog flanger pedals), Uni-V (the classic Japanese phase-vibrato pedal from the late 1960s), Rotary (a rotating-speaker cabinet: horn and drum), Harmonic tremolo (the 1960s brown-face-era harmonic tremolo circuit), Auto-pan and Vibrato (generic).

## Wah, phaser and chorus pedals

| Plugin name | Inspired by | Provenance |
|---|---|---|
| Wah (Auto / manual) | A generic classic wah | You asked for the wah itself, no specific model |
| Phaser | MXR Phase 90 | You asked |
| Chorus | Roland Juno-60 / Juno-106 chorus | You asked |

## v97: Mod FX Fidelity and extra LFO shapes

- **Mod FX Fidelity and extra LFO shapes**: inspired by the Lichtlaerm Nostalgia (eight waveforms, a Fidelity control). From its published description only.

## Envelope Filter

- **Envelope Filter:** a state-variable filter whose cutoff follows the input level (2 ms attack, Decay for the release). Inspired by envelope-filter pedals (Q-Tron style); the circuit was not available, so the sweep range and response are my own choices.

## Mod FX slots (b28 to b30)
Four slots (A to D) share one Type menu: Flanger, Through-zero flanger, Vibrato, Uni-V, Rotary, Auto-pan, Harmonic tremolo, Wobble, Tremolo, Shifted-feedback flanger, Pitch modulation, Stutter, Chorus, Phaser, Lo-Fi warble, four filter sweeps, Auto-wah and Slicer. Chorus, Phaser, Lo-Fi warble and the filter types reuse the stand-alone blocks' engines (so the references above apply); the Slicer is an own design (an eight-step gate with eight patterns); the old stand-alone Chorus, Phaser and Lo-Fi pedals are hidden while off but still exist. Nothing listened to.

# Pitch, octave, harmonizer, synth and noise

## Guitar synth and range notes

- Guitar synth (single-note tracker and synth), pitch range for 7 and 8-string guitars, reverse delay, ping-pong and the extra NAM slots are not modelled on any product.

## Pitch, octave and noise pedals

| Plugin name | Inspired by | Provenance |
|---|---|---|
| Noise pedal | Tallon Electric x Alpha Wolf "The Noise" (a pitch-shifting noise generator). Public descriptions only, so the modes are my interpretation | You asked |
| Noise pedal: Clash layer (scenes Bright / Dark / Full) | Logan Young Audio "The Gross Button" (Nuke / Sicken knobs and three scenes). Intervals and processing are my own | You asked (the plugin's Clash layer was my reading of that idea) |
| Transpose / formant | Generic pitch-shift with formant control | no specific source |
| Pitch pedal (heel/toe Position, Octave up / down, Dive bomb, Bend, Custom) | DigiTech Whammy-style pitch pedal, and the "Wow" pitch pedal in Neural DSP's Gojira plugin | You asked ("especially the whammy/pitch pedal") |
| Octaver (two voices, dry, tone) | The octave pedal in Neural DSP's Gojira plugin, and general analog octavers | My reading of "bring in influence from Gojira" |

## Harmonizers

| Feature | Reference idea |
|---|---|
| Harmonizer (Interval / Scale) | Intelligent-harmony pitch pedals that follow the played note through a key |
| Chord-following harmonizer | Polyphonic harmonizers that keep each note of a chord in key, using a spectral (phase-vocoder) shifter and a multi-note detector of my own design |

# Delay

## Pre-amp delay

| Plugin name | Inspired by | Provenance |
|---|---|---|
| Pre-amp delay (Digital / Analog / Tape / Lo-Fi) | Generic delay types, no specific unit | You asked for lo-fi and tape warble |

## Delay features

| Feature | Reference idea |
|---|---|
| Delay duck | The "ducking delay" found on many studio and boutique delays: echoes drop while you play, bloom when you stop |
| Delay hold (Freeze / Infinite) | Freeze and infinite-repeat footswitch modes on modern delay pedals |
| Delay multi-tap | Rhythmic multi-tap patterns as found on tape and digital multi-head delays |

## Post-amp delay

| Plugin name | Inspired by | Provenance |
|---|---|---|
| Post delay (Ping-Pong, Dual, and so on) | Generic stereo delay types | n/a |

# Reverb

## Reverb shaping and granular notes

- Reverb shaping and the granular additions: you asked for inspiration from the EarthQuaker Devices Afterneath and the Catalinbread Soft Focus. From the guitar.com review and the Reverb.com listing I read, the Afterneath is an "otherworldly" reverb with controls such as Drag (it pulls the reverb's pitch), Diffuse, Size/Space, Mod and Pre-delay, with swelling, shimmering and octave-like textures; the Soft Focus is a soft, diffused reverb/blur pedal for ambient washes. v51's **Drag**, **Diffuse**, **Space**, **Mod**, **Bloom** (a slow swell after a pause) and **Symphony** (a pitch-shifted layer) are my own implementations of those ideas; I do not know either pedal's circuit or exact controls. Treat the match as a loose inspiration.

## Reverb types

| Plugin name | Inspired by | Provenance |
|---|---|---|
| Shimmer reverb | Valhalla Shimmer | You asked |
| Other reverbs (Room, Hall, Plate, Ambient, Cathedral, Spring) | Generic algorithm types | n/a |
| Black Hole reverb type + Gravity | Eventide Space / Blackhole algorithm (inspiration only; own design from the public description of Surge, Linear and Reverse gravity, no circuit or code knowledge) | You asked |

## Optical tremolo and Spring Drive (v83)

Optical: lamp warms in 12 ms and cools in 30 ms, photocell lights in 4 ms and recovers in 45 ms (estimates for a small bulb and cell), light = lamp^1.4, gain = 1 - depth x cell^0.8. Measured swing at depth 0.8: 13.3 dB at 2 Hz, 10.4 dB at 5 Hz, 6.7 dB at 9 Hz (sine: 14.2 dB at all rates). Spring Drive: high-pass 40 to 140 Hz and low-pass 12 kHz down to 5.5 kHz with the knob, then a soft ceiling from 0.55 down to 0.15 with a small bias for even harmonics; small signals pass at unity. These two are shapes I chose, not measurements of a circuit.

## IR Reverb (b2)
Generated IRs are synthesised by IrReverb.h (own code, no sampled data). Loaded IRs are user files only.

## v97: Shimmer octave-down

- **Shimmer octave-down**: inspired by the Lichtlaerm As Above So Below description (octave reverb). Approximation.

## b12 and b13 reverb types

- b12: Reverse, wet tremolo, wet drive, duck release and Focus/Purity were inspired by the feature set of Aberrant DSP's Lair. Inspiration only; the DSP is original and not matched to it.

- b13: ModEcho and Mangled are original designs named after the idea of an echo into a reverb and a reverb with heavy feedback mangling; not matched to any unit.

## Spring return styles (b33)
Seven styles, own filters set from the reverb circuits in the schematics (details in CIRCUITS.md): **Sparkle** from the Fender Twin Reverb (1972) return (0.003 uF into the 100k Reverb pot, about 530 Hz, and a peak near 4 kHz standing for the tank pickup with its 2 nF across it; the peak is an assumption), **Chime** from the Vox AC30 (1978) return (1 nF into 100k, about 1.6 kHz), **Tight** from the Matchless Chieftain (two stages near 330 and 230 Hz, a 284 Hz send high-pass), **Full** from the Orange Rockerverb 50 (about 55 Hz), **Clean** from the Roland JC-120 (about 160 Hz, current-driven so no soft clipping in the driver) and **Tilt** from the Randall RG100ES (a pre-emphasised send and a shelved, treble-tilted return; the real gain curve is far steeper, so this is a tamed version). The Mark IV and Mark V returns were read but not built (they overlap Sparkle and Full). Levels trimmed to within 1 dB of Generic on a noise burst (Chime +3 dB, Tight +1 dB, Tilt -5 dB). Nothing listened to.

## Spring (tank) (b32)
Own design from the physics of a spring reverb tank: two springs of different length, each a delay line in a feedback loop with a long chain of all-pass sections (dispersion: low frequencies travel slower, so a pluck arrives as a falling chirp), a mid-band pickup and a loop low-pass. The tank is not matched to any real unit; the driver stage (Spring Drive) is the older generic one. No schematic of a spring-reverb amp circuit was used. Nothing listened to.

# Cab section

## Built-in cabs, speakers and mics

| Plugin name | Inspired by | Provenance |
|---|---|---|
| Speaker: Modern Tight | Celestion Vintage 30 (V30) | You asked (V30) |
| Speaker: Vintage Crunch | Celestion Greenback | My reading |
| Speaker: Rock Bite | Celestion G12T-75 | My reading |
| Speaker: Smooth Vintage | Celestion Creamback | My reading |
| Speaker: Chimey Alnico | Celestion Alnico Blue | My reading |
| Speaker: Vintage American | Jensen C12N | My reading |
| Speaker: Hi-Fi Full | Electro-Voice EVM12L | My reading |
| Speaker: Modern Scoop, Tight Ceramic, Dark Thump, Hi-Def Light Cone, Smooth Lead, Mid Forward | My own generic tunings for modern metal and versatility, no specific source | n/a |
| Mic: Dynamic Bright | Shure SM57 | You asked (SM57) |
| Mic: Dynamic Full | Sennheiser MD 421 | My reading |
| Mic: Dynamic Presence | Sennheiser e906 | My reading |
| Mic: Ribbon Smooth | Royer R-121 ribbon | My reading |
| Mic: Condenser Detailed | Neumann U87 | My reading |
| Mic: Flat (no mic) | A neutral reference, no mic colouration | n/a |
| Mic: Dynamic Aggressive, Dynamic Punch, Dynamic Scoop, Ribbon Dark, Condenser Warm, Blend | My own generic tunings, no specific source | n/a |
| Cabinet: Oversized 4x12 Straight / Angled (ported) | ML Sound Lab "Reflex" style oversized cab option | You asked |
| Other cabinet shapes (1x12, 2x12, 4x12 straight / angled / open back) | Generic cabinet types | n/a |
| Cab blocks A to D with alignment | Multi-mic cab layering as in common amp sims | You asked |
| SPEAKER page: per-block 5-band EQ, Reso, Phase, Speaker Dynamics, Export cab as IR | Bogren Digital IRDX Studio / IRDX Technology (the idea of shaping and blending IRs, adding speaker behaviour and exporting a result as an IR). What IRDX actually does inside is not documented, so these are my own simple tools in that spirit | You asked ("something like Bogren Digital's IRDX") |

### Cab realism

Real: tight / Real: vintage and Motion are inspired by the idea behind speaker-nonlinearity plug-ins such as Bogren Digital's IRDX Core (a real speaker changes with level, a static impulse response does not) and by Sabol Hill's Lacuna "Current" (a per-band, left/right-independent stochastic drift). Tame is inspired by the idea behind oeksound's soothe3 (a subtractive dynamic EQ that cuts only where a resonance builds up). The implementations are my own simple designs from the public descriptions, not copies and not measured against those products.

### Studio speakers and box resonances (from measured impulse responses)

The seven Studio speakers and the Box resonances were derived from the frequency responses of the seven "MIX" impulse responses in ML Sound Lab's Mega Djent cab pack (the mix-ready files; 44.1 kHz, 24 bit). What was taken: smoothed magnitude curves (fitted with the same few parameters every other built-in speaker uses) and a rounded list of narrow resonance frequencies shared by all seven. No sample data or phase from the pack is in the plugin. Mapping: Studio Standard = DJENT mix, Studio Smooth Tight = FIRE, Studio Bite = GRIND, Studio Full Lead = LEADS, Studio Aged = OLD, Studio Presence = TEETH, Studio Locked Low = TIGHT.

### Heavy, Rhythm and Compact speakers, and rooms

Same method as for the Studio speakers (smoothed magnitude curves fitted with the usual speaker parameters; no sample data or phase kept). Sources, all mixed or mix-EQ'd cab IRs: ML Sound Lab's Zila cab pack (MIX files), the Petrucci rhythm and lead blend IRs (three studios), MDL Tone's Ultimate Heavy Cabs pack (MIX EQ IRs) and Little Cubes 1.5 (the bass cabs were left out). About 40 curves were grouped by similarity and one voicing kept per group.

| Speaker | Taken from |
|---|---|
| Heavy Bright | MDL Ultimate Heavy Cabs, Cab 1 bottom right 57+421 (typical of its first group) |
| Heavy Tight | MDL Ultimate Heavy Cabs, Cab 2 top left 57 (typical of its second group) |
| Studio Hollow Mid | Zila MIX Godzilla |
| Studio Hollow Tight | Zila MIX Quesadilla |
| Rhythm Dark | Petrucci rhythm blend (Liquid studio) |
| Rhythm Smooth | Petrucci rhythm and lead blend (Terminal) |
| Compact Scooped | Little Cubes, Heartbeat v1 |
| Compact Balanced | Little Cubes, Flower v3 |
| Compact Warm | Little Cubes, AFAR RM |
| Compact Dark | Little Cubes, Voda v1 |
| Compact Round Low | Little Cubes, PVD212 v1 |
| Compact Open Air | Little Cubes, AIR115 |

Room types: Studio room is based on the Liquid studio room IR, Large studio room on the Drum World room IR (both from the Petrucci set), and Living room on the Little Cubes bonus room IR. Only their decay times per band, tail tone and level were used.

### Classic and other speakers; removed duplicates

Same method (smoothed magnitude curves fitted with the usual speaker parameters; no sample data or phase kept). The existing speakers were compared with each other and with about 340 candidate curves from Bogren Digital's Jens Bogren packs (Leads, Cleans, Rhythms, Rhythm Downtuned, Tue Madsen, Heaviest), JST cab packs (ANVIL, Conquer All volumes), the Ugritone IR collection and the Catharsis freeware cabs. A candidate was kept only if it fitted well and was at least about 2.4 dB (rms, 60 Hz to 6.5 kHz) from every other speaker. Most Bogren, Tue Madsen, Heaviest and Catharsis curves were within about 1.1 to 1.7 dB of existing speakers and added nothing; some others (ANVIL Steel Lead, a Bohemian lead blend, ANVIL Justified clean, Ugritone PsychoGraffiti) fitted too poorly with this model and were left out.

| Speaker | Closest source type |
|---|---|
| Studio Dark | Heavy Cab pack blends (dark 4x12) |
| Studio Thick Dark, Studio Thick Bright | JST Conquer All EQ'd IRs (1960-type 4x12) |
| Studio Warm Mid | JST Conquer All, MVM-type combo |
| Heavy Hi-Lo | JST ANVIL Downtuned Scoop |
| Classic Dark | Ugritone 3StepSkyn |
| Classic Big Low | Ugritone BlackWizard |
| Classic Mid Focus | Ugritone YearoftheRat |
| Classic Punch | Ugritone UKSteel |

Removed as redundant with Studio Standard or Studio Smooth Tight (see above): Studio Bite (GRIND), Studio Full Lead (LEADS), Studio Aged (OLD), Studio Presence (TEETH), Studio Locked Low (TIGHT) and Studio Hollow Mid (Zila Godzilla). The new cabinet types (1x12 Closed, 1x10 Open, 2x10 Open, 4x10 Closed, 1x15 Closed, 1x8 Small, oversized 2x12) are generic designs of my own, with no specific cabinet modelled.

### Measured mics, position and proximity

Only fitted numbers were kept (no samples, no tables). Mic entries are the average difference from a Shure SM57 on the same cab, over twelve cabs (ten from ML Sound Lab packs, two Kickass Audio Hidden Gem cabs), fitted with a high-pass, a low-pass and three bumps.

| Plugin name | Source |
|---|---|
| Ribbon Open | Royer R-121 (ML packs and Hidden Gem) |
| Condenser Small | Neumann KM184 (ML packs) |
| Condenser Large | AKG C414 (Hidden Gem) |
| Dynamic Large | Sennheiser MD 421 (ML packs and Hidden Gem) |
| Dynamic Long Throw | Electro-Voice RE20 (Hidden Gem) |
| Dynamic Hyper | Beyer M201 (Hidden Gem) |
| Dynamic Wide | Audix i5 (Hidden Gem) |
| Classic Warm Presence | the five ML Sound Lab Mars Hair Metal mixes, which agree within 1 dB |
| Raw Big Low | Ugritone Hardcore IR Pack, KillerChampion CENTER |
| Raw Bright Cap | Ugritone Texas Metal IR Pack, Cowboy CENTER |
| Full-Range Open (v94) | The Lichtlaerm LCFR-150 guitar speaker, from three SM57 close-mic IRs you sent (reference, sweet and in-between spots, averaged; 44.1 kHz, 125 ms). Fitted 0.9 dB rms to the averaged curve. For comparison you sent a V30 with the same mic; it lands within 2.2 dB of the existing Studio Smooth Tight, so I did not add it. The three spots differ by 3 to 6 dB above 5 kHz (reference spot darkest, sweet spot brightest); I kept the average and left that to Position. These are not shipped: the plugin contains my fitted curve, not the IRs. |

Not added because they were redundant (within about 2 dB of an existing speaker after fitting): the Mars Silver mixes, the Boaner and Bgnr Uber mixes (nearest Studio Standard / Rhythm Smooth), Blackback, Cab Pack For Destruction, EVML, Freeman, Zila, the MDL Tone Ultimate Heavy Cabs (all five cabs, T75, EVH G12 and V30 speakers, 57 and 421, and the mix EQ files), the Hidden Gem blends and the freeware JJ edge, ASEM V30 and two-mic files. Mics left out because they sit within about 1 dB of the SM57: SM58, SM7B-style, e906-style and the hypercardioid ribbon.

Position and proximity: five-position sweeps (ML packs, the freeware sets and MDL) show the whole centre-to-edge change is about 40% of the earlier model, mostly between 4 and 6.3 kHz, and the proximity rise is about a third of the earlier model. The REALISTIC switch applies these; sessions from before v71 open with it off.

### IR library

The IR library measures files you choose and compares them with the built-in speakers; it contains no impulse responses and adds no new modelled gear. The Tilt knob is a generic pair of shelving filters.

## Cab pass (cab IR folder analysis)
- Ideal Reference / Warm / Presence / Wide Open: our own smooth speakers, not modeled on any real driver.
- Box by cabinet type: derived from averages of single-mic and mixed IRs of 1x12, 2x12 and 4x10 cabs relative to 4x12 (no pack or product is reproduced).
- Mic models checked against real-mic averages (57, 421, 121, 160, 414): within about 1 dB, so no new mics.

## v97: New speakers

- **New speakers** (fitted to Ownhammer single-mic recordings, no samples kept): American Thin Bite = Jensen P12Q, PA Mid Forward = Altec 417-8H, American Scooped = Jensen C12R, Big Low Bright = JBL D120F, Vintage Hollow Mid = Celestion G12L-35. Fit 0.9 to 1.1 dB rms.

## Big Fifteen speaker

- **Big Fifteen:** an original large-diameter speaker voicing found by searching for a plausible curve at least 3.5 dB (rms, 60 Hz to 6.5 kHz) from every existing speaker; no recordings used.

# EQ and tone matching

## Tone matching

**Tone matching.** The idea of matching a reference recording's tonal balance to your sound comes from the tone-matching features found in other amp software. The method here is my own simple version: an averaged spectrum and a least-squares fit to the nine Post EQ faders.

## Post EQ

| Plugin name | Inspired by | Provenance |
|---|---|---|
| Post EQ (9 bands, slopes, Mod options) | Neural DSP Archetype Nolly-style 9-band EQ idea | My reading |

# Post FX: doubler and widener

## Doubler and widener

| Plugin name | Inspired by | Provenance |
|---|---|---|
| Doubler | Neural DSP-style doubler | You asked |
| Widener | Korneff Audio Wow Thing | You asked |

# Other effects and tools

## Riff recorder

| Feature | Reference idea |
|---|---|
| Riff recorder (DI / Full tone) | Re-amping a recorded DI, and looper-style capture of the finished tone |

## Tremolo, wobble, swell and granular

| Plugin name | Inspired by | Provenance |
|---|---|---|
| Tremolo, Wobble, Swell, Granular | Generic effects, no specific unit (the granular and swell ideas came from your shoegaze / soundscape request) | You asked for the features |

## b15 and b16: Filter, Volume / Pan, Multiband Comp, Gate / Expander and Looper

- b15: Filter, Volume/Pan, Multiband Comp and Gate/Expander are generic designs (state-variable filter, vowel band-passes, tuned combs, Linkwitz-Riley crossovers, a standard gate); not matched to any unit.

- b16: the looper is a generic design (one-button cycle, overdub with undo); not matched to any unit.

## DI Match (b34)
- Own design, not a model of any product. The idea (matching one guitar's tone to another's in front of an amp) is from JST's DI Match; no part of that product was measured or copied.
- Generic pickup models are my approximations of typical behaviour (a resonant low-pass for each pickup type, plus a body shelf and a relative output level), not measurements of particular pickups: single coil resonance about 5.5 kHz, humbucker about 2.8 kHz, P90 about 3.8 kHz, Active about 14 kHz with +6 dB output.
- Measured tones and loaded DIs are compared in 25 third-octave bands from 50 Hz, then fitted with 13 correction filters (low shelf, eleven bells, high shelf), limited to 12 dB each.
- Gate / Expander "Listens to The input" is a plain external-key (sidechain) feed taken at the end of the input stage.

# Factory presets

### Factory presets

You asked for presets "based off of modern/progressive metal bands". The plugin shows only generic preset names. Which band or sound each one is aimed at is listed here. **All of these are my reading of each band's general reputation, tuned by ear and by measurement for level only. They are not captures, and I have not listened to them.** Treat the band as a hint of the direction, not a claim of a match.

| Preset | Aimed at (my reading) |
|---|---|
| Tight Palm-Mute Rhythm, Low-Tuned Chug, Scooped Modern Wall, Clean Boost Staccato | Meshuggah, Periphery, Animals as Leaders, Monuments (djent rhythm) |
| Smooth Sustain Lead, Harmony Lead | Plini, Dream Theater / John Petrucci-style singing leads |
| British Tight Rhythm, Odd-Meter Crunch | Dream Theater, Haken, Tool (progressive rhythm) |
| Dual-Amp Stack | A layered two-amp metal wall, as in many modern prog and metalcore recordings |
| Wah Lead | A classic wah lead (Dream Theater / Petrucci-style, and general prog metal) |
| Octave-Up Squeal Lead, Dive-Bomb Rhythm, Octave-Down Grind, Bend Pedal Expression, Wide Detune Doubler | Gojira (the pitch pedal and octave sounds, which is what you asked to be influenced by) |
| Octave-Up Chord Shimmer | General post-rock / prog ambient chord shimmer |
| Mid-Heavy Groove | Gojira, Lamb of God (groove metal) |
| Wall of Fuzz, Doom Crush | Mastodon, Baroness, Neurosis, Isis (sludge and post-metal) |
| Buzzsaw Rhythm | At the Gates, Dark Tranquillity (melodic death buzzsaw, with the Boss HM-2-style fuzz) |
| Melodic Tremolo-Pick Lead | Opeth, Insomnium (melodic death and prog death leads) |
| Blackened Wash | Alcest, Deafheaven (blackgaze wash) |
| Technical Clarity | Animals as Leaders, Intervals (clear, compressed, articulate lead and rhythm) |
| Classic Crunch, Classic Lead (Open Boost), Edge-of-Breakup Blues | Classic hard rock and blues rock (the plexi / orange direction), with the Open overdrive as a boost |
| Chorused Clean Arpeggios, Phased Clean Riff, Shimmer Pad, Frozen Texture, Slow Swell Strings | Progressive and post-rock cleans (Tool, Intervals, Sleep Token and similar ambient-heavy clean tones) |
| Piezo Acoustic Interlude | The acoustic / piezo interlude sound used on many prog records |

# General notes

## Notes on the reference rigs and the generic names

**Boost, Mod FX, reverb and scapes.** The plugin's UI, tooltips and README use generic names; the sounds below are what each was designed to sit near. They are my own designs from the sound descriptions, not copies of circuits.

**Tones check, from the rigs the players are known for.** Sources are the Premier Guitar rig rundowns and other pages I read on the day (see the links in my reply); rigs change, so treat these as snapshots:
- Deftones (Stephen Carpenter): Fractal Axe-Fx II (earlier a Marshall JMP-1 preamp into an EL34 100-watt power amp); 7- and 8-string low tunings (for example F#-B-E-A-D-G-B-E on 8 strings).
- Yvette Young: a Vox AC30 (top boost channel, high input, amp reverb on); compressor (EarthQuaker Warden), chorus/vibrato (Walrus Julia), analog delay (MXR Carbon Copy Deluxe), reverbs (Walrus Fathom, Meris Mercury7), octave (MXR Bass Octave Deluxe), a ten-band EQ (MXR), plus other ambient pedals; heavy modulation; F-A-C-G-B-E tuning.
- Polyphia: per the tone guide I read, a clean, versatile amp (modeler or solid state) with a light transparent overdrive, compression, a slight mid boost, delay and reverb, and chorus or phaser; news that they have used Marshall amps.
- Meshuggah: Fractal Axe-Fx Ultra units (2013), 8-strings tuned a half-step down (Ibanez M8M / Stone Man).
- Periphery: Fractal Axe-Fx II XL into a stereo power amp and Zilla 2x12 cabs (Celestion Creamback H and Vintage 30), 6- and 7-strings.
- Spiritbox (Mike Stringer): a modified 5150 / 6505+ style head (rhythm and lead channels), a Tube Screamer-style pedal with low drive and high level to tighten the lows, a built-in gate, IRs, drop F# and 8-strings.

### Overall idea

The project as a whole started from "something like Neural DSP's Archetype Plini". Hand-built DSP was your choice (not neural capture).

### Not a model of anything specific

Tuner, noise gate, pickup de-noise, input meter / output meter, limiter, and the oversampling and mode settings are ordinary utilities.

### If you want the names back in the plugin

The plugin keeps generic names so the product is safe to release. If you only want them temporarily for yourself, ask and I can add them to the hover tooltips (one line each, such as "Speaker: Modern Tight (V30-style)") and I will keep that change in a clearly separate patch so it is easy to remove again before a release.

## b36: circuit audit and capture refits (measured, not listened to)
- **Wah (Classic circuit voice).** Sweep 450 Hz to 1.6 kHz, the source page quotes up to 18 dB of amplification (not separately modelled); resonance from the measured behaviour of a real pedal (Q about 9.4 rocked back, 4.0 mid, 1.9 rocked forward at pole frequencies of 464, 838 and 2252 Hz). Sources: electrosmash.com (Crybaby GCB-95 and Vox V847 analyses), a published re-engineering paper on the same circuit, and a Stanford CCRMA paper that measured a pedal at three positions. The published pages give no numeric Q, so the Q law is the measured one, not a derivation.
- **Phase 90.** Four first-order stages, C = 47 nF, JFET-controlled resistance; the sweep is linear in frequency from 330 Hz to 3.3 kHz (geometric middle 1040 Hz), with the circuit's feedback; the LFO is a triangle. Source: electrosmash.com MXR Phase 90 analysis. Rate range widened to 0.02 to 20 Hz.
- **Stack Drive and Blues Driver.** The old model sent both pedals through limiters that were shared between stages or clamped at 30 rails, so the Stack Drive's second stage (up to several hundred volts) collapsed and the Blues Driver lost about 11 dB. Both are fixed; the small-signal gain of the Blues Driver at Gain 0 is now about +7 dB (the published figure). Screamer tone-stage input resistance is 909 ohm (1k in parallel with the 10k bias resistor).
- **FET and Opto compressors.** FET: 50 us attack detector, 5 us floor, release 50 ms to 1.1 s, program dependent. Opto: about 10 ms average attack, two-stage release (about 60 ms, then a slow tail up to about 5 s that grows with how long and how hard it squeezed). Published figures come from a vendor page and a 1966 manual; one source calls the opto attack "essentially instantaneous", so the vendor value is used as the centre.
- **Juno-style chorus.** Rates 0.513, 0.863 and 9.75 Hz; delays 1.66 to 5.35 ms (I and II) and 3.3 to 3.7 ms (I+II), from a published analysis of recordings of a unit. The service notes list 0.5, 0.83 and 1 Hz; the analysis suspects the 1 Hz is a typo.
- **Capture refits.** Master Cranker Stock (to a JCM800 with mods, Gain 2 / noon / 10) and Harmonic Haze Clean (to a clean amp-only capture): the fits use the six-level sweep (fundamental gain, 2nd and 3rd harmonic against level) rather than EQ. Cranker Stock is within about 4 dB of the capture's gain at all measured points and follows its saturation shape; the harmonic mix in the middle of the knob is a few dB off. The new Mayer-style captures are analysed (JSON) but no voicing is built from them yet.
