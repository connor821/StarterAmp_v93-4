# Amp sources: what each amp and voicing is built from

Brand names appear here (as in MODELED_GEAR.md and CIRCUITS.md). Nothing has been listened to against real gear.

Key for how each voicing was built:
- **S** = schematic values drive the circuit-derived numbers (stack, cathodes, supply, feedback). "S-part" = some values read, the rest estimated.
- **C** = tone and/or gain structure fitted to captures (NAM files or pink-noise/sine measurements).
- **M** = manual or description only (control names, channel layout, no component values).
- **R** = my own tuning from the amp's reputation. No measurement.

| # | Amp | Based on | Voicing: how built |
|---|---|---|---|
| 0 | Twin Sparkle | Fender Twin Reverb-style clean | Sparkly: S-part (1972 Twin stack, stage and power curves). Edge push: R. Boutique: R (Matchless / Two-Rock idea). Studio: R |
| 1 | Chime Thirty | Vox AC30-style | Chime: S-part (1978 schematic curves; top-boost stack uses standard published values). Treble boost: S-part. Normal: R. Jangle: R |
| 2 | Brown Eye | Friedman BE-100-style | Tight: C (BE channel captures: gain range, tone). Heavy: C (HBE captures: gain range, tone). Brown: C partial (the HBE Fat switch capture's tone; gain range is a guess between the two). Mid push: C partial (voice-left capture's tone). Refit in b26 from the capture sets plus the BE-100 schematic (hot-rod stage gain structure, EL34 stiff supply, heavy feedback). Not listened to |
| 3 | Slow Burn | Soldano SLO-100-style | Smooth, Lead: S-part (SLO-100 stack, 39k feedback family). Crunch, Sustain: R. No captures |
| 4 | Modern Maul | Dual Rectifier-style | Scooped, Clarity, Vintage: S-part (two Dual Rec schematic revisions) + C (Mesa captures). Chunk: R |
| 5 | Acoustic | Piezo / magnetic acoustic amp | Own design, R |
| 6 | Club Crunch | Plexi-style, Orange-ish pushed voicing | Crunch, Jumped: S-part (50 W British schematic check, 1959 schematic values). Pushed, Bright: R |
| 7 | Bumble Lead | Dumble-style | Jazz, Rock, Lead: S-part (Dumble schematics: cathodes, bright cap, OD coupling, supply) + C (Dumble ODS and Steel String Singer captures, no cab). Blues: R, nudged against the captures |
| 8 | 5555 Block | Peavey 5150-style, Invective-style | Raw: S-part (5150 II power board only). Modern, Crunch, Buzz: R |
| 9 | Tweedle Dee | 5E3 tweed-style | Soft, Cranked: S (5E3 schematic built into the tweed model). Four-ten (Bassman idea), Gritty: R |
| 10 | Jade Chorus | JC-120-style solid state | All R. JC-120 service notes received in b22: power amp and specs read (see CIRCUITS.md), preamp not yet transcribed or applied |
| 11 | Marque Dream | Mesa Mark IIC+ / Mark-series | Rhythm, Lead, Vintage: S-part (IIC+ first stage, bright cap, supply; Mark V supply and feedback) + C (Mesa captures). Singing: R |
| 12 | Citrus Doom | Orange-style, Sunn-style stack | All R. (An Orange schematic set was used for Matt's Amp and Marmalade, not here) |
| 13 | Revved Up | Revv Generator-style | Tight, Lead: C (plain Revv captures: clipping, tone). Crunch: R. Polished: R (RedSeven Leviathan idea) |
| 14 | Digital Dread | The modern hyper-processed plugin sound (Thall-style and Big Bottom-style amps) | All R. A Big Bottom-style NAM capture was only used to check gain structure (odd versus even harmonics) and moved the Solid Metal hybrid voicing, not this amp. Not fitted to any capture yet. Thall-style captures expected |
| 15 | Master Cranker | JCM800-style | Stock: S-part (2203 stack and first stage). Modded: C (Friedman Jose captures, no cab) + the three Jose-mod schematics (see CIRCUITS.md). Cranked: R. Boosted: R (Purple Nightmare reputation) |
| 16 | Harmonic Haze | Bogner Shiva-style | Lead, Crunch, Clean, Boost: S-part (Shiva schematic cathodes and tone network) + M. No captures |
| 17 | Diesel Fuel | Diezel VH4-style | Lead, High gain, Crunch: C (b21: 47 VH4 captures, harmonics and tone fitted; Lead = channel 3, High gain = channel 4, Crunch = channel 2). Open follows Lead. Based on a service manual (no schematic) + R |
| 18 | Wreck Stack | ENGL Savage 120-style | S-part (supply, 6550 pair, presence network, first-stage cathodes) for all voicings; the voicings themselves R |
| 19 | Power Brawl | ENGL Powerball II-style | M (manual) + S (schematic: plate supply only, b18) + R |
| 20 | Cutaway | Lichtlaerm Prometheus-style | Lead, Sludge: C (two Prometheus captures: even harmonics, EQ fitted to both). Clean, Brilliant: R. Shop page only, no schematic | (b21: harmonic balance partly fitted to 5 more Prometheus captures)
| 21 | Hi-Wat Headroom | Hiwatt DR103-style | Normal, Brilliant: S-part (tone stack solved from the schematic, supply). Linked: S-part. Open: R |
| 22 | Console DI | Neve 1073-style channel | Console, Pushed, Funk, Air: M (manual specs: 110 Hz and 12 kHz shelves) + R. Not a solved circuit |
| 23 | Plex Appeal | Plexi 100 W-style | Stock, Jumped: S-part (Marshall-type stack). Modded, Modded + Jumped: S-part (Jose-mod effects from the ampmodlab pages; the Plexi Jose schematic was added in v0.2.x, see CIRCUITS.md) |
| 24 | Matt's Amp | Orange OR120 / GRO 100 / GT120-style | Graphic, Heavy, Bright, Mix: S-part (three Orange tone networks solved) + R for gain, clipping and sag |
| 25 | Marmalade | Orange Rockerverb-style | Crunch, Heavy, Clean: S-part (Rockerverb, Dual Terror, Dark Terror and Jim Root sheets) + C (Rockerverb 100 and MkIII captures). Sharp: R, derived |
| 26 | Eclipse | Modern extreme-metal 100 W head (Nameless-based captures) | Hi Pulled, Hi Pushed, Lo Pushed, Lo Pulled: M (manual settings) + C (about 20 captures, no cab; component values estimated, knob positions unknown) |
| 27 | Marque Five | Mesa Mark V-style, three channels | Clean, Crunch, Lead, Extreme: S-part (Mark V preamp, supply, feedback) + C (Mark V captures: Clean/Fat, Ch 2, Ch 3 MK-IIC+, Ch 3 Ext) |
| 28 | Dual Thirty | Matchless DC30-style, two channels | Channel 1, Channel 2, Pushed, Cut: S-part (DC30 gain structure, supply, bright cap read; first-stage cathode value unreadable, assumed) + R tone; no captures. Clubman and Chieftain schematics received, not built (EL34 family overlap) |
| 29 | Solid Metal | Solid-state / hybrid 90s metal (op-amp + 12AX7 hybrid head and all-JFET head) | Hybrid Hi-Gain, Hybrid Classic OD, JFET Lead, JFET Crunch: S-part (both schematics: stage types, clipping elements, supply) + R tone; one NAM capture used only to compare gain structure. Component values and tone-network behaviour estimated |

## Gaps worth filling (my read)

- **No captures yet:** Twin Sparkle, Chime Thirty, Slow Burn, Club Crunch, 5555 Block, Tweedle Dee, Jade Chorus, Citrus Doom, Digital Dread, Harmonic Haze, Diesel Fuel, Wreck Stack, Power Brawl, Hi-Wat Headroom, Matt's Amp, Plex Appeal.
- **Voicings that are only R inside an otherwise measured amp:** Brown Eye (Heavy, Brown, Mid push), Revved Up (Crunch, Polished), Master Cranker (Cranked, Boosted), Cutaway (Clean, Brilliant), Marque Dream (Singing), Bumble Lead (Blues).
- **Missing or not yet applied:** Diezel VH4 (no readable schematic). Received but not applied: Friedman BE-100 (read in full), JC-120 (power amp read, preamp left to transcribe), Mark IV lead (read only).
| 30 | Cedar Twelve | Small blackface-style combo (two 6V6, about 22 W; Big Spring voicing: a 40 W two-6L6 combo with a 15-inch speaker) | Blackface, Spanky, Cranked, Big Spring: R. The Twin-style amp's preamp and tone-stack idea with a smaller supply and power stage; the supply numbers are estimates. Measured only against the Twin Sparkle voicings (compression and tone), no captures, no schematic |
| 31 | Triple Blend | Three amps in parallel inside one amp (a warm 15-inch combo, a Dumble-style lead amp, an open clean-headroom head) | PLACEHOLDER parts: Warm = Cedar Twelve Big Spring (R), Smooth = Bumble Lead Rock voicing (S-part + C, existing), Open = Hi-Wat Headroom Normal (S-part, existing). Part levels and the four blend voicings (Three-in-One, Warm, Open, Singing) are R. To be fitted to captures of the three-amp rig at three Gain settings |

- **Soldano SLO-100 output transformer (replacement-part specs, b27):** 2.2 kohm primary, 18.5 H, 4/8/16 ohm taps, 100 W; logged in TRANSFORMERS.md, not applied (corner about 19 Hz, inaudible).
