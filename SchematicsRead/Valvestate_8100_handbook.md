# Marshall Valvestate handbook (8100 / 8200 heads; 8280, 8240, 8080, 8040, 8020, 8010 combos)

Source: `vs1-1..8.png`. Eight pages of the Marshall Amplification plc "Valvestate Handbook", dated Nov '98 on the cover. Pages 1 to 7 are text; page 8 is a front-panel line drawing of all six panels (double width).

**There is no schematic in this document.** There are no component values, no voltages, no tube operating points, no tone-stack networks, no feedback or clipping-circuit details, no supply ratings. Nothing here can be computed. All that follows is what the handbook states in words, plus what the front-panel drawing shows. Anything about topology below the level of "bipolar versus MOSFET power stage" would be invention, so it is not given. (CIRCUITS.md line about 258 already says the same: Valvestate has no schematic in the set.)

## 1. Model line-up (pages 1, 2 and 4)

| model | power (page 4 wording) | type | power devices (page 2) |
|---|---|---|---|
| 8010 | 10 W | combo | bipolar |
| 8020 | 20 W | combo | bipolar |
| 8040 | 40 W | combo | bipolar |
| 8080 | 80 W | combo | bipolar |
| 8100 | 100 W | head | bipolar |
| 8240 | 40 W per side | stereo chorus combo (twin power stages) | bipolar |
| 8280 | 80 W per side | bi-chorus combo | MOSFET |
| 8200 | 100 W per side | bi-chorus head | MOSFET |

Page 2 (INNOVATION IN AMPLIFIER TECHNOLOGY), the technology claims in the handbook's own words:
- "ValvestateTM is able to do this by simulating the performance of a valve power stage. Using Bi-Polar devices in models 8010, 8020, 8040, 8080, 8100 and 8240 and MOSFET devices in models 8280 and 8200 in a new constant current configuration produces the type of high impedance output levels normally only delivered by a valve amplifier."
- Claimed consequences: "speaker reaction similar to that found in valve equipment. Ringing harmonics, rich bass response and sensitivity".
- Pre-amp: models 8040, 8080, 8100, 8240, 8280 and 8200 carry a Marshall **ECC 83 / 12AX7** in the pre-amp "to give even greater dynamics by providing real valve overdrive". By omission the 8010 and 8020 have no tube.
- A "special Contour control" lets you "focus the overdrive from mellow blues right through to all out screaming metal".
- Stereo chorus and reverb are "totally analogue" (8240, 8280, 8200; reverb on the others as listed below).

What this tells the plugin (the only hard facts): (1) the power stage is meant to have a **high output impedance** (current-drive character) in all the models, bipolar or MOSFET; (2) one 12AX7 triode stage (the handbook says "a valve", singular) sits in the boost channel's drive path; (3) the rest of the preamp is not described and is, by the wording ("solid-state" is never used), not specified at all.

## 2. Channels and controls by model (pages 3 to 7, panel drawing page 8)

All pages describe controls only by function. Where the handbook states an effect, it is quoted.

**Normal channel (Channel 1)** on 8200, 8280, 8240, 8100, 8080:
- Gain, Clean/Crunch push switch, Bass, Middle, Treble (full 3-band).
- Clean/Crunch: "In Clean mode, sounds remain clean, even at high gain levels. When switched to Crunch the amount of crunch increases as you wind the gain up." The glossary adds that crunch "is achieved by turning the normal channel gain to maximum or just below" and "models 8080, 8240, 8100, 8280 & 8200 also have a switchable crunch selection".
- 8040 normal channel: Gain, Bass, Middle, Treble; Gain "remains clean up to half way, then bringing in blues to crunch rhythm sounds as the gain increases". 8020 normal: Gain, Treble, Bass (no mid); "remains clean up to half way, then turns to slight distortion, with full crunch reached when on maximum gain".

**Boost channel (Channel 2)** on 8200, 8280, 8240, 8100, 8080:
- Gain (ECC83/12AX7 valve drive), OD1/OD2 switch, Bass, Middle, Treble, **Contour**, Volume.
- OD1 / OD2 (page 4, item 9): "Manually switches from creamy 'classic' OD1 to screaming 'Hi Gain' OD2 distortion modes." Item 8: "On OD1, it fattens the bluesy distortion and on OD2 goes from rock overdrive to full metallic thrash as you turn it up."
- Contour (item 13): "This sweeping control shifts the mid-band voicing of the boost channel to give amazing versatility to your overdrive sounds. When used in conjunction with the 3 band EQ, this control is devastating!" Glossary: "Our special mid band shift contour control changes the voicing of the boost channel". The panel drawing labels its ends **LOW** and **HIGH**. It is described as a mid-band shift (a sweeping mid voicing), not a depth-only mid control.
- 8040 boost: Gain, Contour, Bass, Treble, Volume, Master Reverb (no Middle, no OD1/OD2 switch drawn). 8020 boost: Gain, Contour, **Presence** (the panel drawing shows a scale with end marks that I cannot read cleanly (?); "Adds extra cut and bite to boost selections"), Volume, Master Reverb. 8010: one Gain, a single Boost Select switch ("Normal (clean & crunch sounds) or Boost (overdrive sounds)"), Master Volume, Bass, Middle, Treble; "Full Gain = Full Shred!".

**Master section**:
- 8100 / 8080: Effects Send jack, Effects Level switch (0 dB for rack units, -20 dB for pedals; "the loop comes in after the amps' pre-amp distortion circuitry"), Effects Return jack, Effects Mix (dry to effects), Master Reverb (one reverb for both channels), Master Volume, Line Out, Footswitch jack (reverb and channel). The panel drawing also shows a second small switch next to Treble on the 8100/8080 (the "boost channel select" LED and a "channel select" switch).
- 8240: stereo send / left and right return, Chorus depth and rate, Master Volume.
- 8280 / 8200: Parallel stereo loop (left return, right return, send) and series mono loop (return, send), Mode A/B chorus switch ("Mode A, flange type chorus, to Mode B, traditional chorus"), separate Chorus Rate and Depth for the Normal and the Boost channel, two parallel speaker jacks per side.
- Line Out is described as "Filtered low level signal ... Special filtering allows you to link from this jack into recording equipment" ("specially filtered to enable you to link directly into recording or PA mixing equipment and still achieve the Valvestate guitar sound"), i.e. a speaker-simulating filter is in the Line Out path of all models. Its shape is not given.
- Minimum load: "the combined total impedance of speaker cabinets should not be less than 4 Ohms" (8100 rear panel and 8280/8200 outputs). The 8100's own cabinet is "the 140 Watt compact 4x12 model 8412". The 8240 also feeds its two internal speakers from the two power stages.
- Pre-amp Out / Power Amp In sockets on the 8040 allow use of the power stage alone. The handbook warning list says never to connect the negative side of the speaker output to earth (the amp is not ground-referenced at the speaker).
- Footswitch parts: P801 (single, channel switching), P802 (two-way, reverb and channel), P803 (chorus and channel).
- 8010 and 8020 have a headphone jack that mutes the speaker.

## 3. What can be said about the sound (from the handbook only)

- Two voicings of overdrive in the boost channel (OD1 "creamy classic", OD2 "screaming Hi Gain"), selected by one switch; the single 12AX7 stage is stated to be the source of the "real valve overdrive". Everything else about the clipping is silent.
- Contour is a swept mid-band voicing; its direction and frequency range are not given.
- The power stage is a high-impedance (current-like) output stage, intended to behave like a tube amp into a speaker.
- Normal channel has two modes: Clean (stays clean at any gain per the handbook) and Crunch (gain adds distortion, "extra gain built into" the channel).
- Bass, Middle, Treble (and Presence on 8020) are described only as "interactive" three-band EQs.

## 4. What could not be read or is absent

- Everything quantitative: no schematics, values, voltages, corner frequencies, gains, clip thresholds, damping factors, supply sag. The handbook is a user manual.
- The 8020 Presence scale marks and the 8010 Master/Gain taper (not stated).
- Whether the 8240 and 8280 stereo power stages share the same output topology (not stated beyond "twin ValvestateTM power stages" for chorus).
- Pages are 585 x 827 px (page 8 is 1170 x 827): the page-8 panel drawing text is small but legible; page 2 to 7 text is fully legible.
