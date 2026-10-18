# Starter Amp: version history

This file keeps the old per-version notes (v50 to v72) for reference. The README, MANUAL and MODELED_GEAR are organised by section; read those first. Page and tab names in these old notes are the names of the time (PRE FX, BOOST, MOD FX, SPEAKER and so on): since v73 the pages are INPUT, GAIN, MOD, PITCH, AMP, CAB, EQ, DELAY, SCAPES and POST FX.

## v94 (edited in place in this folder)
**Sputter fixed in Circuit-derived mode (measured, not heard).** Cause: the grid-blocking stage shapes (each preamp stage's bias wandered by up to 2.0 and leaked back over 45 ms), not the power section or supply. In a plucked-note test on four high-gain amps, held-note level wobble on a high E was 0.88 dB (Classic 0.34 dB) and a high note carried about 20 dB more sub-150 Hz energy than Classic. Now: smooth knee, half the charging rate, cap 0.5 and a 15 ms leak: wobble 0.40 dB, sub energy about 6 dB lower (still about 14 dB above Classic, which is the asymmetric shapes themselves). Low-note (82 Hz) results were noisy in my metric and did not clearly change. Classic is unaffected.
**Grip now works on the hand-tuned Transparent too** (low cut 120 -> 240 Hz ahead of the clipper, Edge shelf from 1.5 kHz, +1.5 dB). My own recipe, not measured against anything.
**The original tones are back, and the circuit work is still there as a choice.** Each amp (A and B) has its Mode menu again: **Classic** (the original voicing, now the default) or **Circuit-derived** (circuit stage shapes, power section, supply, component-value voicing and the Low-Watt Combo's built-in Tweed Tone). In my render test Classic is bit-identical to v92's Classic on nine amps. Old sessions that stored a Mode keep it; sessions made in v93 have none stored, so they load as Classic and sound different from how they did in v93.
The pedal drives get a **CIRCUIT MODE** switch (odCircuit, one switch for both drives): off = the original hand-tuned Transparent, Tight, Screamer and Distortion (bit-identical to v92 in my test); on = the circuit models from v93. The other pedals stay circuit models. Grip needs Circuit Mode (Transparent). The Buzzsaw stays the circuit version; I did not restore its old hand-tuned one.
**Drive Mix** (odMix, od2Mix, default 1) and **Transparent Grip**: see v93 notes above; Mix works in both modes.
**Riff recorder:** the panel's update timer now always runs, `prepare` no longer wipes a recording when the host re-prepares at the same rate, and mono inputs are handled. In my test harness record, stop and play all worked; I could not reproduce the failure you saw, so this is not confirmed fixed.
**Not done / not measured:** I have not listened to anything. Whether Classic removes the "loose, fizzy, sputtery" sound is untested by ear; I could only show that it is the same code and output as v92.

## v93
Drive 2: a full second copy of the OVERDRIVE block (new parameters od2On, od2Gain, od2Tone, od2Edge, od2Style, od2Range, od2Attack, od2Gate, od2Volume, od2Auto, od2Clip, od2Voice, od2Bass, od2Mid, od2MidFreq, od2MidMod, od2Route; defaults match Drive 1 except od2On = off). Series: Drive 1 runs on its own and Drive 2 sits in the amp's oversampled path. Parallel: both run on their own from the same input and are added at -3 dB each. Drive 2 alone is bit-identical to Drive 1 alone with the same settings (checked at 4x, 2x and no oversampling); 8 styles x 2 routes with Drive 1 on: no NaN. Transparent is now the Centaur-type circuit (Circuit::KlonCircuit; estimates listed in MODELED_GEAR). The Clean and Treble boosts are circuit models in the Boost section; Dyna (OTA) is a new Comp style (appended). Removed: the Circuit Mode switch (Tight, Screamer, Distortion, Op-Amp Distortion, Stack Drive are always circuit versions), the amp Mode menus (circuit-derived only), the Full-circuit amp code is unreachable, and the Style entries 8 to 10 from v92 (Treble Boost, Clean Boost, Sustain Comp). Fuzz 2: a full copy of the fuzz block (new parameters fuzz2On, fuzz2Style ... fuzz2Rate, fuzz2BuzzLow, fuzz2BuzzHigh, fuzz2Route; fuzz2On defaults off). It runs in the Fuzz slot of the effect order, straight after Fuzz 1 (Series) or beside it (Parallel, -3 dB each); Fuzz 2 alone is bit-identical to Fuzz 1 alone with the same settings (7 styles x 3 oversampling modes), 9 styles x 2 routes with Auto Level on both: no NaN. It does not have its own light on the chain strip. New file CIRCUITS.md lists every circuit received and what was done with it. Not compared bit-for-bit with v92, because the amp path and the Transparent style changed on purpose.

**Tweed built into Low-Watt Combo.** From the 5E3 schematic: the single Tone control (Treble knob) as a solved circuit, 25 uF cathode corners, the 5Y3 supply with its 4.7k x 16 uF screen stage. Level matched to the old amp by measurement (within about 1 dB across Gain). Not listened to. Fixed on the way: very low high-pass corners (under about 20 Hz) made the Best (4x) quality mode go silent on this amp; the corners are now held at 25 Hz or higher.

**Saturated Lead and Classic Rock** take the SLO-100 and 1959 supplies and (Saturated Lead) the lead channel's cathode corners; levels re-checked, within about 0.4 dB of before.


**Buzzsaw is now a circuit (from the HM-2 schematic you sent).** Two gain stages, op-amp, clipper and the two-band tone network, solved as a circuit; Low and High are about -20 to +20 dB, and the Tone knob is hidden for this style. It sounds different from the old Buzzsaw, and existing sessions will change. Level matched to the old one by measurement; not listened to. Transistor and op-amp behaviour are estimates.

**Buzzsaw gets the modded-HM-2 options (from the N.E.W. DIY HM-2 schematic you sent).** The single tone knob is split into Mid and High (the two resonant branches each get their own 10k pot; set them equal for something close to the original). SOFT and HARD switches take the feedback diodes and the shunt diodes in and out (both on = the original). Blend mixes in a clean copy of the input (the board's non-inverting stage, gain 2) and Invert flips it. New parameters: buzzMid, buzzSoft, buzzHard, buzzBlend, buzzPol and the fuzz2Buzz equivalents. Checked for stability only, not listened to; Blend is a plain mix, so with Invert off or on it will comb against the distorted path. Not done: the Medusa's own extras (Cut, Presence, mid sweep, Grind, Modern gain, gate), because that manual has no schematic.

**Boost Mix.** New bsMix knob (default 1 = as before). Fuzz and Comp already had Mix; Drive 1 and Drive 2 do not yet.

**Fuzz Factory: not changed.** I built a transistor-level solver from your schematic but it was not stable enough (Silicon mostly dead, Germanium latches at full Drive), so Factory keeps its earlier model. The Fender 5E3, Marshall 1959 and Soldano SLO-100 files are recorded in CIRCUITS.md only.


## v92
Five new OD styles appended to the Style menu (values 6 to 10): Treble Boost, Clean Boost, Op-Amp Distortion, Stack Drive, Sustain Comp, from published analyses (see MODELED_GEAR for what is read and what is estimated). New measured Auto Level rows. No new parameters; existing styles untouched.

## v91
Distortion circuit mode (the existing Circuit Mode switch now also acts on the Distortion style): booster, op-amp stage, hard diode clipper and passive tone network from published component values; new Auto Level row measured for Silicon clipping; no clean path. New appended parameter wahVoice (Generic / Classic circuit, default Generic): Classic circuit fixes the sweep at 450 Hz to 1.6 kHz. Defaults unchanged; Classic-mode renders bit-identical to v61 on the 48 compatibility renders.

## v90
Read a second Mark-series schematic (eight pages). Its supply matches the earlier one, so the creamy lead amp's supply now uses 448 V and 5.6k x 33 uF (0.185 s). Its speaker-feedback network (56k with 0.005 uF, 3.3k with 0.047 uF, 1.5k) is the same as the one partly read in v89, so that amp moved from the light-feedback family to the British-lead one in Circuit-derived mode; its power-section trim re-matched (-1.50 to -0.50 dB). Classic mode and defaults unchanged (48 / 48 compatibility renders bit-identical to v61).

## v89
Circuit-derived mode now uses component values read from three uploaded schematics, for two amps. Modern High-Gain: first-stage cathode corner (1.8k / 1 uF) and a supply from the schematic (460 V, 2.7k x 30 uF, 0.081 s slow time constant; source resistance estimated). Creamy Lead: first-stage cathode corner (1.6k / 0.47 uF; tighter low end than the previous by-ear voicing), a 120 pF bright cap on a 1M pot, and a supply from the schematic (480 V, 5.6k x 30 uF, 0.17 s; peak current and transformer saturation estimated). Level trims re-matched (Creamy Lead -1.6 dB, Modern High-Gain +0.2 dB). Classic mode and defaults unchanged. Feedback networks, the lead channels and the graphic EQ were read but not applied; details in MODELED_GEAR.md.

## v88
Circuit-derived is now the default Mode for Amp A and Amp B (ampMode and ampBMode default 1). New sessions and factory presets start in it; sessions saved before v87 are migrated as in v87 (so an old session that was Classic stays Classic), and sessions saved by v87 keep their saved mode. The state marker ampV87 is now also set on new sessions so a fresh session is not migrated back to Classic. Every non-acoustic, non-glass, non-digital amp model already used the derived power section, supply droop, feedback loop and stage curves; the component-value preamp voicing exists only for the American combo and the 50 W British models (Clean, British Hot-Rod, Classic Rock, Master-Volume Rock), because those are the schematics I have read. Not extended to the others: I would be inventing values. Classic mode is bit-identical to v61 on the 48 compatibility renders. Level difference of the new default against Classic (guitar-like test signal, gain 0.2 / 0.5 / 0.8): mostly within about 2 dB; the largest are Boutique Lead -5.2 dB and Low-Watt Combo -4.4 dB at gain 0.8, more supply sag and compression than Classic. Not retuned.

## v87
One Mode menu per amp replaces Circuit Amp, Stage Model and Derived (new appended parameters ampMode: Classic / Circuit-derived / Full circuit, and ampBMode: Classic / Circuit-derived; default Classic, 48 compatibility renders bit-identical to v61). Circuit-derived = stage curves plus the full derived level (3). Removed from the interface and from the sound: Dual voice, Power scaling, Output class, Rectifier (both amps) and the Metal Sag knob. The Stack menu is now As voiced, Neutral and the seven passive stacks (ampStack values change: old 0 to 0, American to the American combo stack, British to the British 50 W stack, Neutral to 1, passive stacks minus two). Older sessions are migrated on load (marker property ampV87): Circuit Amp on to Full circuit; Stage Model or Derived on to Circuit-derived; Power Half/Low adds 0.15/0.30 Power Drive; Silicon rectifier multiplies Sag by 0.35; Metal Sag scales Sag on the metal amps; factory presets updated to match. Timing (8 s audio): Classic 0.12 to 0.16 s, Circuit-derived 0.17 to 0.20 s, Full circuit about 3.6 s; reported latency 3 samples in every mode. Master knob only shows in Full circuit mode. Stress run: 150 random-setting runs, no NaN.

## v86
Power stage inside its negative-feedback loop (Derived menu, any level above off): the output stage is solved per sample inside its global feedback loop (Newton's method, three steps), with the loop gain taken from each family's feedback network and the Feel setting (loose, normal, tight). Small signals pass as before; the stage's own distortion is reduced by the loop gain until it runs out of swing, so tight feedback stays clean longer and then clips harder, and loose feedback breaks up earlier and softer; crossover distortion of a cold bias is reduced as on a real amp. Feel no longer adds its own drive nudge when Derived is on (the loop does that). Level trims re-matched. Costs about 0.1 s more per 8 s of audio when the power stage is clipping (0.56 s against 0.44 s, with the stage model on). All 48 compatibility renders are bit-identical to v61.

## v85
Dual voice (Amp A, appended parameters ampDual and ampDualBal, default off): the first gain stage runs as two parallel triode halves with their own volume pots, a full-gain voice and a lower-gain voice (ratio about 0.35, my choice: the source drawing is hand-drawn and does not show it), blended by Voice Mix with audio-taper weights and level-matched; it uses the plain asymmetric curve, not the Stage Model's grid blocking. Measured at Gain 0.6 on the 80s rock model the effect is small (under 0.2 dB gain and under 0.2 dB non-fundamental energy), because the later stages clip the signal anyway. With Derived on, the Power Tube choice no longer adds its own bass, mid, treble and presence offsets (the power section models that); its drive and sag effects stay. All 48 compatibility renders are bit-identical to v61.

## v84
Two more passive tone stacks appended to the Stack menus (Amp A and Amp B; ampStack and ampBStack values 9 and 10; default unchanged): "high-gain, 680 pF" and "high-gain, 500 pF", read from a two-channel high-gain amp's schematic (220k treble, 1M bass, 22k mid, 47.5k slope, 0.02 uF caps; the treble cap is the only difference). All 48 compatibility renders are bit-identical to v61. No other change.

## v83
Circuit-derived amp levels, and two effects. (1) New Amp A / Amp B menu "Derived": off (classic, the default), power section, power + supply, power + supply + voicing (new parameters ampDerived and ampBDerived, appended). Power section: the Feel and Presence nudges are replaced by the closed-loop response of the output stage with its negative-feedback network, presence control, output transformer and a speaker impedance model, solved offline for five amp families and stored as a small table (PowerSectionTable.h, four filters at the oversampled rate). Supply: a two-time-constant droop (reservoir and filter stages) plus output-transformer low-end saturation, with depth from each amp's supply values (supply values are estimates, flagged in MODELED_GEAR.md). Voicing: cathode-bypass and coupling corner frequencies and the bright-cap shelf worked out from component values, for the American combo and 50 W British models only. Every level is trimmed so the guitar test signal is as loud as classic (measured: exact). (2) Tremolo gets an Optical shape (bulb and photocell: lamp and cell time constants, shallower at fast rates) appended to Tremolo Shape. (3) Reverb gets a Spring Drive knob (the spring's driver: soft lopsided clip, thin lows, rolled-off top; Spring type only; off at 0). The effects are not part of the amp models. All 48 compatibility renders are bit-identical to v61; no extra CPU (0.52 s against 0.53 s for 8 s of audio).

## v82
Housekeeping: two code comments in ToneStackTmb.h named real amps; the comments now describe the stacks neutrally (no behavior change). Measurements recorded in MODELED_GEAR.md of how far the classic Screamer and Tight overdrive modes sit from their Circuit modes.

## v81
Five passive tone stacks added to the Stack menu of Amp A and Amp B (appended choices; ampStack and ampBStack values 4 to 8; default unchanged): ToneStackTmb.h solves the bass / mid / treble circuit from component values (numerator and denominator fitted from nodal solves, then a bilinear transform to a third-order filter), PassiveStack.h runs it with coefficients interpolated across each block, and gain is matched at the middle setting. Fixes the Circuit Amp's bass control direction (it was reversed in v79). All 48 compatibility renders are bit-identical to v61; 120 stack / knob-extreme / block-size runs and a knob-twiddling test gave no NaN or runaway output.

## v80
Stage Model menu for Amp A and Amp B (new parameters ampStageModel and ampBStageModel, default classic, appended): the preamp stages and power stage can use clipping curves derived from tube-circuit simulation (StageCurves.h, generated with the TubeNet solver; StageShaper.h applies them): asymmetric per-stage curves by stage type and preamp tube, grid blocking, and push-pull power-tube curves per power tube with a hot-bias variant. Levels trimmed per model to match the classic shaping. Same CPU as classic. All 48 compatibility renders are bit-identical to v61; 192 extreme-setting runs (both amps, all models, tubes and bias extremes) produced no NaN or runaway output.

## v79
First whole-amp circuit model (Circuit Amp menu on the AMP page, default Off): a 50 W master-volume lead amp solved from its schematic with a nodal Newton solver (preamp stages, bright volume, tone stack, master volume, phase inverter, EL34 push-pull power stage with output transformer, negative feedback and presence, supply sag). New parameters ampCircuit and ampMaster are appended; all 48 compatibility renders are bit-identical to v61. Runs at twice the host rate. The tone filters, Auto Level and Metal controls are skipped while it is on. New helper headers TubeNet.h (tube tables and solver) and TubeAmps.h (the amp).

## v78
Three more Wall circuit versions, from schematics: "Circuit (variant B, 500 pF)" and "(variant C, 470 pF)" (the later build of the same fuzz: 0.1 uF coupling capacitors, 0.047 uF in series with the diodes, 390 ohm emitters, 12k collectors, 20k tone resistor; gains and poles from a transistor small-signal model calibrated to the first circuit's published figures) and "Circuit (op-amp build)" (inverting booster, a Sallen-Key low-pass at about 1 kHz with Q about 1, a sustain divider, a 741 clipper with three silicon diodes each way, a Muff-style tone network, Tone Bypass). All appended to the Wall Version menu; defaults unchanged (48 comparison renders still bit-identical to v61). Levels match the old Wall within about 1 dB at Gain 0.5. v77 is unchanged.

## v77
Circuit modes. Tight and Screamer overdrives get a "Circuit mode" switch (off by default) that solves the actual schematic: component-value filters, a diode pair solved exactly in the feedback loop (anti-aliased), the op-amp's gain-bandwidth limit and a new "None (op-amp rails)" clipping choice. Drive keeps a wide range (extended below the stock pedal, full stock range above). The Wall fuzz gets a "Circuit (from the schematic)" version: booster, two diode-limited stages, the tone network solved exactly, output stage. All defaults unchanged: the 48 comparison renders are bit-identical to v61. Auto-level tables were measured for the circuit modes. Aliasing of the circuit modes is about the same as the old ones (Tight at full Drive 3 dB worse).

## v76
Three near-duplicate voicings were retuned so they sound clearly different (measured with a gain, input-level and tone sweep): Precision High-Gain "Lead" (now tighter, brighter and more defined than Creamy Lead "Lead") and Low-Watt Combo "Four-ten" (now fuller, brighter and cleaner than "Soft"). Precision High-Gain "Polished" (4th voicing) was also a near-copy of "Tight"; it now starts from the open Crunch channel (hi-fi, more headroom, more dynamic), at least 1.2 dB from every other voicing in the sweep. All three stay within about 1 dB of the old loudness at Gain 0.5. Saved sessions that use exactly those three voicings will sound different; everything else is unchanged (48 test renders bit-identical to v61).

## v75
Tone Lab: match a recording from a DI file and a target file (searches amp, gain and tone, cab, compressor, overdrive, boost and Post EQ on a private copy of the plugin). Sound unchanged.

## v74
Six compressor styles (Standard, FET, Opto, Variable-mu, Sustainer, Bus) with four matching saturations, two new Boost modes (Mid, FET) and four new chorus voicings (Dimension, Tri-chorus, Ensemble, Detune). All appended to the menus; defaults unchanged, so the sound of existing sessions is the same (48 test renders bit-identical to v61).

## v73
Window reorganised into ten pages by job, TIDY VIEW (switched-off pedals collapse to small tiles), HELP window, drag-and-drop reordering of effects, CAB page split into two sub-tabs, docs reorganised by section. The sound is unchanged: 48 test renders are bit-identical to v61.

## What is new in v72

- **Windows builds for beta testers.** No sound changes. `.github/workflows/windows.yml` builds a Windows zip (standalone app plus VST3, 64-bit) in GitHub's cloud, so you do not need a Windows PC. See `WINDOWS_BUILD.md`. The AU format is Mac only and is skipped on Windows. The Windows runtime is linked in, so testers install nothing extra.

## What was new in v71

More realistic mics and mic position, from measured recordings, plus three speaker voicings.
- **Seven new mics** (Ribbon Open, Condenser Small, Condenser Large, Dynamic Large, Dynamic Long Throw, Dynamic Hyper, Dynamic Wide), each fitted to the average difference from a plain dynamic mic over twelve real cabs. Across cabs the mic differences agreed to within about 1 to 1.5 dB, so they behave like a mic, not like one cab. The older mics are unchanged.
- **REALISTIC switch** (CAB page). On, the Position and Distance controls change the sound by the amount recordings show (about 40% of the earlier swing in the highs, about a third of the earlier proximity boost). Off is the earlier behaviour. New sessions start On; sessions saved before v71 open Off and sound exactly the same as before.
- **The Mic menu now has two folders** (Hand-built, Measured). **Three more speakers** (Classic and Raw (bold) folders). Classic Warm Presence comes from five recordings that agree within 1 dB. Raw Big Low and Raw Bright Cap are raw centre-of-cap voicings and are deliberately bold.
- **What was not added.** Everything else you sent measured within about 2 dB of a speaker that was already there once fitted, so adding it would only lengthen the menu. The details are in MODELED_GEAR.md.

## What is new in v70

An IR library window, an audition path, and two more knobs on every cab block.
- **IR LIBRARY** (button on the CAB page, next to ALIGN NOW). Press ADD FOLDER and pick a folder of WAV or AIFF impulse responses; subfolders are included. The window shows your folders as a tree (with All IRs and Favourites at the top), the files in the selected folder with a small picture of each one's frequency curve, and a larger plot and readout for the selected file. Press RESCAN after adding files. The scan is remembered, so the second time is quick. Files longer than 5 seconds are skipped (they are loops or songs, not cab IRs).
- **What the readout tells you.** The curve is the file's average frequency response, smoothed to 1/6 octave and centred on the 500 Hz to 2 kHz level. The text gives the balance of four bands against that level, the low resonance, where the top rolls off, and the **closest built-in speaker** with how far apart they are (about 1.25 dB or less = the same sound; 1.25 to 2.4 dB = close; more than 2.4 dB = a voicing none of the built-in speakers cover). The menu above the plot can compare the file with your current IR in any cab block instead.
- **Duplicates.** A file within 1 dB of an earlier file in the list is marked with a wavy equals sign and hidden while HIDE DUPLICATES is on. This compares the average curve only, so two files can still differ in fine detail and feel; turn HIDE DUPLICATES off to see everything. HIDE removes a single file from the list by hand (SHOW HIDDEN brings them back), and FAVOURITE stars it. Favourites, hidden files and your folder list are kept for all sessions, not in a preset.
- **AUDITION.** With AUDITION on, the signal from your amp goes through the selected file in place of the cab blocks, so you hear it in your own chain with the same level scaling the blocks use. Your blocks are not changed. Selecting another file (click, or the up and down keys, or PREV and NEXT) loads it at once. Turning AUDITION off, closing the window or switching it off from the library window puts the cab blocks back. SEND TO A, B, C or D loads the selected file into that block, sets the block to IR file and switches it on.
- **Two more knobs per cab block, ten in all.** **Delay** (0 to 10 ms, in 0.01 ms steps) moved from the advanced page to the main knobs: it is the manual alignment. ALIGN NOW and AUTO ALIGN still set it for you; you can fine-tune it by hand while watching the IR ALIGNMENT display. **Tilt** (-12 to +12 dB) is new: positive makes this block brighter by lifting the top and cutting the bottom around 800 Hz by the same amount, negative does the opposite. 0 does nothing, so old sessions sound the same.
- **Old sessions sound the same:** in my test all 48 reference renders from v61 are still bit-identical.
- **Not in this version:** fitting a file to an adjustable speaker. The library only measures and compares.

## What is new in v69

More cabinets and more speakers, and a tidier Speaker menu. Same method as v67 and v68: I measure each response, fit the usual few speaker parameters to it, and keep only those numbers; no impulse response or sample data is inside the plugin.
- **Seven new Cabinet types**, in folders by speaker count: **1x** (1x12 Open, 1x12 Closed, 1x10 Open, 1x8 Small, 1x15 Closed), **2x** (2x12 Open, 2x12 Closed, 2x10 Open), **4x** (4x12 Closed and the other 4x cabs, including 4x10 Closed) and **Oversized**. The new ones are 1x12 Closed, 1x10 Open, 2x10 Open, 4x10 Closed, 1x15 Closed, 1x8 Small and an oversized 2x12. The first four cabinets keep their old positions, so old sessions keep their cabinet.
- **Six speakers removed** because they were too close to Studio Standard and Studio Smooth Tight (within about 1.25 dB on average, which is below what you can hear on a guitar): Studio Bite, Studio Full Lead, Studio Aged, Studio Presence, Studio Locked Low and Studio Hollow Mid. Studio Hollow Tight is now just **Studio Hollow**. To get close to the removed ones, start from Studio Standard and use **Bite** for upper-mid bite, **Tight** for a tighter low end, and **Box** for the cabinet resonances.
- **Nine new speakers**, each at least about 2.4 dB away from every other speaker, so each sounds like something new: **Studio Dark**, **Studio Thick Dark**, **Studio Thick Bright**, **Studio Warm Mid**, **Heavy Hi-Lo**, and four in a new **Classic (older styles)** folder: **Classic Dark**, **Classic Big Low**, **Classic Mid Focus** and **Classic Punch**. The Classic ones come from raw close-mic recordings, so they carry some close-mic brightness; use Position, Distance and a darker mic if you want them smoother.
- **What I found redundant:** most of the larger collections of lead, clean, rhythm and downtuned cab recordings landed within about 1.1 to 1.7 dB of voicings already in the menu, so I did not add them. This is a fact about the average frequency curve; recordings can still differ in fine detail and in feel.
- **Still close:** a few older pairs are within about 1.1 to 1.4 dB of each other (Vintage American and Hi-Def Light Cone, Chimey Alnico and Mid Forward, Rock Bite and Tight Ceramic, Modern Tight and Tight Ceramic). I left them in because they predate this check and your sessions may use them.
- **Index shifts:** speakers were removed and renamed, so a saved session or a host automation lane that points at a v67 or v68 speaker number may land on a different speaker. Re-pick the speaker if a loaded sound changes. Speakers 0 to 12 (the original hand-built ones) are unchanged.

## What is new in v68

More cab sounds built from measured impulse responses (same method as v67: I measure each response, fit the same few speaker parameters to it, and keep only those numbers; no impulse response or sample data is inside the plugin), plus modelled rooms.
- **Twelve new speakers** from four more sets of mixed cab recordings. After measuring about 40 responses I grouped them by how close their curves are and kept one voicing per group, so the menu stays short: **Heavy Bright** and **Heavy Tight** (two heavy 4x12 voicings), **Studio Hollow Mid** and **Studio Hollow Tight** (a scooped-mid pair), **Rhythm Dark** and **Rhythm Smooth** (darker 4x12 rhythm voicings with a lower top cut near 4 kHz) and six **Compact** voicings from small cabs: **Scooped** (dip at 400 to 600 Hz), **Balanced**, **Warm**, **Dark**, **Round Low** and **Open Air** (the only one with usable top above 6 kHz). They match their recordings within about 1 dB on average (60 Hz to 7 kHz), worst about 1.4 dB. Several recordings were within 1 dB of the seven Studio speakers from v67, so I did not add copies of them.
- **The Speaker menu is now in folders.** **Realistic** holds the hand-built models grouped by cab type (1x12, 2x12, 4x12, 4x12 oversized), and **Mix ready** holds the fitted ones (4x12, Compact cabs). The cab type is what each speaker is typically used for; you can use any speaker with any Cabinet. The menu order does not change what is saved: old sessions load the same speaker.
- **Room types** (CAB panel, menu above the Room knob): **Original** is the old dark ambience, **Studio room**, **Large studio room** and **Living room** are modelled on three room-microphone recordings: decay about 0.2 to 0.3 s for the studio rooms and about 0.35 to 0.5 s for the living room, more bass than treble in the tail, and a dark top. **Room** sets how much. A modelled room makes the cab impulse response longer (16384 samples instead of 4096), which uses a little more CPU.
- **Old sessions sound the same:** Room Type defaults to Original, and in my test all 48 reference renders from v61 are still bit-identical.

## What is new in v67

Built-in cab sounds based on measurements, all adjustable. I measured seven mixed 4x12 impulse responses (the "mix-ready" ones from a metal cab pack) and used the results to build adjustable models. No impulse response, wave data or table from the pack is inside the plugin. What I took from the measurements is a list of numbers: the shape of each response (about 20 numbers per voicing) and a rounded list of narrow resonance frequencies. Everything is generated by the same builder as before.
- **Seven new speakers** (at the end of the Speaker menu): Studio Standard, Studio Smooth Tight, Studio Bite, Studio Full Lead, Studio Aged, Studio Presence and Studio Locked Low. Each was fitted to one of the seven mixes (the fit is within about 0.8 dB on average between 60 Hz and 7 kHz). They differ the way the mixes do: the low-end corner (66 to 130 Hz), the upper-mid bite around 2.3 kHz and 3.7 kHz, and how smooth the top is. All of them have the steep top cut near 5.5 kHz that the measured cabs share. They are fitted for the **Flat (no mic)** mic and the **4x12 Closed** cabinet; any other mic, position or cabinet stacks on top, so you can still shape them.
- **Three new knobs on each cab block** (model page): **Tight** (right = less below about 150 Hz, left = fuller), **Bite** (right = more at 2.4 and 3.8 kHz, left = smoother) and **Box**. **Box** adds the narrow peaks and notches a real cabinet has between 600 Hz and 4.5 kHz, which a smooth model lacks (19 of them, from the measurements, rounded and with the deepest notches limited to 6 dB). At full Box the fine detail of the built-in cab correlates 0.77 with the measured one (0.13 with Box off). The resonances move with the **Cabinet** menu: smaller boxes ring higher, the oversized ones lower. Tame (v65) can still smooth any that bother you.
- **Old sessions sound the same.** The three knobs are centred (Tight, Bite) or off (Box) by default, and in my test all 48 reference renders from v61 are still bit-identical. New knobs only do something when you move them.
- **What I did not do:** I did not change the mic or position models. The measured IRs are cut off above about 6 kHz and cannot say how the high end changes with mic position. Across their positions A to E the measured IRs gain 1 to 6 dB around 5 kHz and lose 1 to 3.5 dB below 400 Hz, which is the same direction as the built-in Position control.

## What is new in v66

**Two new fuzz styles (PRE FX page, FUZZ pedal).**
- **Nuke (wave-shaper):** Gain drives the signal through a curve; **Shape** morphs through five curve families (soft, sine-fold, odd-harmonic, staircase, asymmetric fold), blending neighbours. **Crush** adds bit-depth and sample-rate reduction together (0 = off). **Tone** is a slightly resonant two-pole output filter. Mix and Level work as on every fuzz. It is my own simple design from a general description of wave-table distortion.
- **Crusher (bits and rate):** a bit-crusher with drive in front. **Bits** (16 down to 1), **Rate** (sample-rate reduction, aliasing up to about 1/32), **Gain** (drive into the crusher) and **Tone** (output filter that tames the harsh top).
- Both run inside the fuzz's existing oversampling, so the Crusher's aliasing is partly the point and partly softened; set the quality choice that suits you. In the harness every setting gives finite output, and the new styles land within about 3 dB of the other fuzz styles at default settings. Nothing else changed: the 48-file compatibility check against v61 is bit-identical (the new fuzz parameters are new, so old sessions load with the old styles). I have not listened to them.

## What is new in v65

**Cabs that behave less like a fixed snapshot (SPEAKER page, SPEAKER DYNAMICS panel).** The panel stays small: a mode menu and three knobs, with the extras behind ADV.
- **Mode:** the two old modes are unchanged. **Real: tight** and **Real: vintage** do the same and add three level-dependent cone behaviours that Amount scales: the bass resonance rises and fades as the cone moves further; the top fades while the lows work hard; and the 1.5 to 4 kHz band breaks up on one side only (the lows are left alone).
- **Tame:** turns down resonances in the cab sound (fizz, honk, boxiness) only while they stick out of the local spectrum. It works after the cab, on built-in cabs and loaded impulse responses, with no latency. In my test a smooth noise signal changed by 0.04 dB overall, while two tones standing out of it were turned down by about 4 to 7 dB at full Tame. Behind ADV: **Listen to cuts** plays what Tame is removing.
- **Motion:** slow random movement in six broad bands, different left and right and bigger the harder you play, so the cab never sounds exactly the same twice. Behind ADV: **Rate**.
- Everything is off by default; with Amount, Tame and Motion at 0 the sound is bit-identical to v64 (the 48-file compatibility check against v61 is identical). The three together cost about 2% of one core in my test.
- I built these from general descriptions of the idea (a real speaker changes with level; an impulse response does not), not from measurements of any product or speaker. I have not listened to them, and I would value your reference audio to tune them.

## What is new in v64

**A Tube Driver style in the Overdrive section.** The Overdrive Style menu has a sixth entry, Tube Driver: a full-range, warm, creamy tube-style drive. A lot of the clean signal stays under it at low Drive, so it is a thick, open boost, and it turns compressed and creamy when pushed: a lead boost for rhythm and solos. Knobs: **Drive**, **Bass** (a low cut on the gain path: tight at 0, full-range at 1), **Treble** (tilts the top after the clipper) and the shared Volume; **Bright** is behind ADV. Its clipper is a soft, slightly asymmetric tube-style curve with plenty of headroom. Auto Level has a measured row for it; it behaves like the Screamer's (within about 2 dB of the matched level across Drive). Nothing else changed: the 48-file compatibility check against v61 is bit-identical. I have not listened to it.

## What is new in v63

**A fourth voicing on every electric amp.** The Voicing menu now has four positions (Amp A and Amp B). Each new voicing starts from one of the amp's own three and changes gain, tightness, sag, tone and top end, with its level trim measured so that at Gain 0.5 it is about as loud as the amp's first voicing. Old sessions and presets are unaffected (positions 1 to 3 are unchanged, bit for bit).

| Amp | 4th voicing | What it does |
|---|---|---|
| Clean | **Studio** | Hi-fi: more headroom, a smoother low mid, a brighter top, less sag |
| Vintage Chime | **Jangle** | A brighter, more open chime with less compression |
| British Hot-Rod | **Mid push** | A forward, nasal upper mid and less bass (for riffs that sit in a mix) |
| Saturated Lead | **Sustain** | More gain and sag, a rolled-off top, long singing notes |
| Modern High-Gain | **Chunk** | Tighter, a firmer low end and a forward upper mid |
| Classic Rock | **Bright** | More gain and presence, a little tighter |
| Boutique Lead | **Blues** | Breaks up earlier, with forward upper mids and more sag |
| Tight Metal | **Buzz** | Tighter and more gain, a scooped low mid and a bright, edgy top |
| Low-Watt Combo | **Gritty** | A cranked, dark, saggy breakup |
| Glass Clean | **Shimmer** | A very bright, scooped, glassy clean |
| Creamy Lead | **Singing** | A smoother, rounder, more vocal lead |
| Doom Stack | **Fuzz** | A fuzzy, dark, very saturated wall with more bias |
| Precision High-Gain | **Polished** | A clear, tight, modern voicing with big headroom and a firm low end (a big-power-tube style) |
| Digital Extreme | **Lead** | The scoop filled in, a touch less tight, for solos and leads |
| Master-Volume Rock | **Boosted** | A boosted preamp: tighter, more gain, a forward upper mid |

Auto Level has a new row for each. In the test harness every new voicing lands on the same loudness at every Gain setting, as the first voicing of the same amp does. I chose these voicings by general rules of thumb and measurement; I have not listened to them. Tell me which ones miss and I will retune them.

## What is new in v62

**A new amp: Master-Volume Rock (Amp A and Amp B).** The 80s hard-rock voicing that was the third position of Classic Rock is now an amp of its own, with three voicings: **Stock** (exactly the old Classic Rock "Master volume" sound), **Modded** (a modified version of the same kind of amp: an extra cascaded gain stage, so it is tighter, smoother, more compressed and more saturated, with a lower-mid cut and a hotter upper mid) and **Cranked** (the master volume wide open: the power stage does the clipping, so it is looser, saggier, louder and a little darker). It has no Modern Metal panel or Focus.

**Classic Rock has a new third voicing, Jumped:** both channels linked, the old trick for a fatter, hotter sound: fuller low mids, more gain and sag, a softer top than Crunch.

**Old sessions and presets still sound the same.** A session or preset saved with Classic Rock on its third voicing is moved to the new amp's Stock voicing when it loads (Amp A and Amp B separately). The Master Volume 80s Rhythm factory preset now uses the new amp.

**The amp menus are in folders by gain level:** Acoustic at the top, then Clean (Glass Clean, Clean, Vintage Chime), Low / mid gain (Low-Watt Combo, Boutique Lead, Classic Rock, Master-Volume Rock, Saturated Lead) and High gain (Hot-Rod, Creamy Lead, Modern High-Gain, Tight Metal, Precision High-Gain, Digital Extreme, Doom Stack). The parameter values did not change, so automation and old sessions are unaffected. Amp B's menu has the same folders.

**Auto Level** has new rows for the three new voicings and the changed Classic Rock voicing; in the test harness every one lands on the same loudness at every Gain setting. With Auto Level off, the new voicings sit at about the same level as Stock at Gain 0.5.

**What I checked (not listened to):** the build is clean; the 48-file compatibility test against v61 is bit-identical; Stock is bit-identical to the v61 Classic Rock third voicing; the 0-failure test run is unchanged. I have not run or listened to any of this on a Mac.

## What is new in v61

**Two new amps, three voicings each.** The Amp menu now has fourteen electric amps. Both new amps have the Modern Metal panel and Focus, and both are also in the Amp B menu (its last two entries). They are hand-tuned approximations built from general reputation, not circuit models, and I have not listened to them.

| Amp | Voicing 1 | Voicing 2 | Voicing 3 |
|---|---|---|---|
| **Precision High-Gain (tight, modern)**: a modern boutique high-gain amp, tight and articulate with a firm low end and a smooth top | **Tight**: the main high-gain channel, a scooped low mid and a 1.8 kHz bite | **Lead**: more gain, a mid hump near 1.3 kHz, more sag and sustain | **Crunch**: an open, dynamic, lower-gain channel (two stages) |
| **Digital Extreme (brutal, digital)**: a modern digital-style amp for extreme styles, with very high gain, almost no sag and hard clipping | **Thall**: ultra-tight, a deep cut at 350 Hz, a sharp 2.4 kHz edge | **Industrial**: harsh and gritty, fuzzier clipping, strong upper mids, more top-end fizz | **Cavern**: huge, slow and dark, with a lot of sag and headroom, for drop tunings |

What I measured (plucked test signal, Amp Gain 0.5, overdrive off): the two voicings Tight and Lead compress about 10 dB and Crunch about 5 dB; Thall has the most energy between 500 Hz and 3.5 kHz (22% and 11%), Industrial the most above 3.5 kHz (6.9%), and Cavern keeps about 91% of its energy under 500 Hz. Every voicing sits at about the same level as the other amps at Gain 0.5 (within about 0.2 dB in the harness), and each has its own Auto Level row, built the same way as the others (so expect about 1 to 2 dB of residual across the knob on real playing). Focus tightens Digital Extreme about half as strongly as the other amps, because it is already very tight (at Focus 7 its level stays within 0.1 dB, where the others drop about 0.5 dB).

**A new look.**
- **Pedal art.** Every pedal panel now has a bold enclosure band in the pedal's colour with a title, a small line-art illustration (an eye, a sun, a moon and stars, waves, mountains, a spiral, dots or a lightning bolt, chosen by the pedal), a screw in each top corner and a faint watermark of the same art in the lower corner. The drawings are my own simple paths.
- **Amp illustration.** The right side of the AMP page, which was empty, now shows an amp head on a four-speaker cab drawn in the selected amp's colours, cloth and plate, with its name engraved and a lit power lamp. It follows the model on the Amp A and Amp B tabs and is hidden for the Acoustic model and on the NAM tab.
- **Two new amp looks:** Precision High-Gain is warm graphite with copper piping, an amber-lettered plate and pinstripe cloth; Digital Extreme is blue-black with ice-white piping, a cyan-lettered plate and a dot-matrix cloth.
- The Modern Metal panel's title is now just "MODERN METAL" (it covers more amps than its old title listed).
- No controls were moved or removed. Saved sessions and presets load unchanged.

**Documentation.** There is now a comprehensive `MANUAL.md` (installing, the window, every page, the amps in depth, presets, MIDI, levels, recipes, troubleshooting, and a control reference generated from the plug-in's own hover help) and a one-page `QUICKSTART.md`.

- **Six new factory presets:** Precision Tight Rhythm, Precision Lead With Delay, Precision Dynamic Crunch, Thall Chug, Industrial Grind, Cavern Doom Slam.

**How it was checked.** Everything builds with no errors in my test build against JUCE. The 48-case sample-by-sample comparison against v60 (every old amp, voicing and effect setting) is bit-identical. Level measurements: the six new voicings land at -21.3 to -21.4 dB at Gain 0.5 in the harness, where the older high-gain amps sit at -21.2 to -21.6. The 36 existing amp and voicing combinations give identical level, compression and band-share figures to v60, and the 48-case comparison above covers the effects. I have not run the plug-in on a Mac or listened to it.

## What is new in v60

**The Voicing switch has a third position on every electric amp.** The switch is now a three-position menu that names the position ("Voicing: Four-ten"). Positions 1 and 2 are the two voicings every amp already had, exactly as before; position 3 is new. Old sessions and presets load into positions 1 and 2 unchanged (my 48-case comparison against v59 is bit-identical, and so is the harmonizer comparison). Amp B has the same menu, with names for its own model. These are hand-tuned approximations built from the general reputation of each sound, not circuit models, and I have not listened to them. Each has its own level trim and its own Auto Level row, so it sits at about the same loudness as the others.

| Amp | Position 1 / 2 / **3 (new)** | What the new one is |
|---|---|---|
| Clean | Sparkly / Edge push / **Boutique** | Fuller low mids, tighter lows, more compression, a touch-sensitive chime |
| Vintage Chime | Chime / Treble boost / **Normal** | A darker, rounder, thicker channel without the top boost |
| Hot-Rod | Tight / Heavy / **Brown** | A thicker, saggier, mid-rich, looser vintage-hot-rod crunch |
| Saturated Lead | Smooth / Lead / **Crunch** | A low-gain channel: open and dynamic, much less compression |
| Modern High-Gain | Scooped / Clarity / **Vintage** | Spongy: more sag, looser and fatter lows, a softer attack |
| Classic Rock | Crunch / Pushed / **Jumped** | (v62) Both channels linked: fatter, hotter, more sag. The old "Master volume" voicing is now the Master-Volume Rock amp |
| Master-Volume Rock | Stock / Modded / Cranked | (v62) An 80s hard-rock amp: Stock is the old Master volume voicing, Modded adds a gain stage, Cranked is the master wide open |
| Boutique Lead | Jazz / Rock / **Lead** | A boosted lead mode: more compression and sustain, a vocal mid hump |
| Tight Metal | Raw / Modern / **Crunch** | A low-gain, palm-mute-friendly rhythm channel with the same tight lows |
| Low-Watt Combo | Soft / Cranked / **Four-ten** | A 4x10-style voicing: more headroom, bigger lows, mid-forward |
| Glass Clean | Glass / Warm / **Funk** | Tight, percussive and scooped, with a stiff low end |
| Creamy Lead | Rhythm / Lead / **Vintage** | An early-style sound: lower gain, thick creamy mids, a smaller sweeter low end |
| Doom Stack | Stoner / Wall / **Bass stack** | A bass-amp-style stack: massive low mids, a cleaner top, grind only when pushed |

What I measured (plucked test signal, Amp Gain 0.5, overdrive off): the new Crunch voicings compress far less than the high-gain ones (about 4 to 5 dB of gain compression against 9 to 10 dB for the Lead, Creamy Rhythm and Tight Metal voicings); the Bass stack is the cleanest of the Doom Stack's three (about 4 dB against 7 to 9 dB); Master volume compresses more than the Classic Rock Crunch (about 9 dB against 5 dB); Funk has less low-mid and more upper-mid energy than Glass; and the Modern High-Gain Vintage and Hot-Rod Brown put more of their energy in the low mids. The other differences (sag, feel) I cannot measure with this signal.

**Bug fix: Focus on the Creamy Lead.** The Focus knob (the low-tuning tightening) did nothing on the Creamy Lead since v55, because a check on the amp model was testing the wrong value. It now works on that amp the way it does on the Tight Metal. Nothing else changes; the Creamy Lead sounds the same as before at Focus 0.

- **Two new factory presets:** Four-Ten Crunch and Master Volume 80s Rhythm.

## What is new in v59

**Five amp-wide switches, and a streamlined Amp panel.** The new switches work on every electric amp model, for Amp A and for Amp B, and each has a level compensation (measured as the average over twelve amps at three gain settings), so the switch changes the feel and tone rather than the loudness. All of them start neutral, so old sessions and presets sound exactly the same (my 48-case comparison against v58 is bit-identical). These are hand-built approximations, not circuit models, and I have not listened to them. Their effect is subtle on a clean amp and clearer as the amp is pushed.

- **Tone stack:** As voiced, American (scooped mids, more bass and treble), British (mid-forward, a forward upper-mid push) or Neutral (about 70% of the amp's own baked-in tone shape taken out). Bass, Low Mid, High Mid and Treble still work on top.
- **Feel (power-amp feedback):** Loose (more sag, fuller and looser lows, less presence), Normal (the amp as voiced) or Tight (less sag, tighter lows and a little more presence).
- **Power (power scaling):** Full power is the amp as voiced. Half and Low power break the power stage up and sag sooner, like a lower-wattage amp cranked.
- **Class:** As voiced, Class A (more sag and compression, more even harmonics, a slightly softer top) or Class AB (a little less sag, tighter, a touch more presence).
- **Deep:** a low-end boost of about 2.5 dB into the tone stack, with a little more low end allowed into the first stage.

**The Amp panel is streamlined, like the Fuzz and Overdrive panels.**
- **Always visible:** the model, VOICING and AUTO LEVEL on the first row; then BRIGHT, Tone stack, Feel and Power on the second; then the knobs Gain, Bass, Low Mid, High Mid, Treble and Presence (plus Focus on the amps that have it).
- **Behind ADV:** the preamp tube, power tube and rectifier menus (they were always visible since v52; they are back behind ADV now that the switches that change the feel most are in front), Class, DEEP, the oversampling quality, and the knobs Bias, Power Drive, Resonance and Sag.
- Amp B has the same layout. The Modern Metal panel is unchanged.
- **Two new factory presets:** Half-Power Class A Chime and Tight Deep British Rhythm.

**Not built.** A third voicing choice per amp, a built-in chorus on Glass Clean and a graphic EQ on Creamy Lead were on my list; they would need new stages inside the amp, so I have left them out (the Chorus pedal and the Boost EQ already do those jobs).

## What is new in v58

**Two new Overdrive styles: Screamer and Distortion.** The pedal's style menu now has five styles (Transparent, Tight, Open, Screamer, Distortion), and the panel works like the Fuzz panel: the controls you reach for are always visible and the extras sit behind the panel's **ADV** button (remembered with the session). Old sessions and presets are unchanged (my 48-case comparison against v57 is bit-identical). These are hand-built approximations, not circuit models, and I have not listened to them.

- **Top row (every style):** OVERDRIVE ON, the style menu and AUTO LEVEL.
- **Screamer.** A versatile mid-hump drive: a clean path blended with a clipped gain path that has a low cut. *Always visible:* the **Clipping** menu, the **Voicing** menu, and the knobs Drive, Tone, Mid and Volume. *Behind ADV:* Bass, Mid Freq and Bright.
  - **Clipping** (also on the Distortion): Silicon (classic), LED (more headroom, louder, more open), Germanium (clips early and softly, compressed) and Asymmetric (the two halves clip differently, giving even harmonics).
  - **Voicing:** Classic (the familiar mid hump and full gain), Mild (leaner, about 3 dB less gain, no extra hump) and Open (brighter, about 2 dB more gain, looser lows).
  - **Bass:** the low cut into the clipper. Low is tight and mid-focused (the classic sound, cutting at about 700 Hz); high is full (about 70 Hz).
  - **Mid and Mid Freq:** cut or boost (about 8 dB either way, around the voicing's own hump) and move where it works, from about 350 Hz up to about 2 kHz.
  - **Bright:** extra treble fed into the clipper.
- **Distortion.** A hard-clipped distortion: all the signal goes through a high-gain stage into hard clipping (a harder knee than the Screamer's). *Always visible:* the **Clipping** menu and the knobs Dist, Tone and Volume. *Behind ADV:* the **Mids** menu (Scoop = the stock mid dip, Flat, Boost = a mid hump) and Bright. Tone tilts the highs around 1.2 kHz.
- **AUTO LEVEL** has measured rows for both new styles at each clipping choice, at the settings the panel starts at. It is about as accurate as the existing styles (within roughly 1 to 2 dB at the higher Drive settings, and it does not follow Bass, Mid or Tone changes).
- **Level:** LED clipping is the loudest and Germanium the quietest; Auto Level compensates for the clipping choice. The Screamer's voicing switch also changes its gain a little (Mild less, Open more).
- **Two new factory presets:** Screamer Mid-Boost Lead and Asymmetric Distortion Crunch.

## What is new in v57

**The Fuzz panel is one page again.** The MAIN / OPTIONS tabs are gone. Every option still exists with the same parameters, so v56 sessions and presets load and sound the same. The panel now works like the compressor and wah panels: the controls you reach for are always visible, and the rarely-touched ones sit behind the panel's **ADV** button (remembered with the session).

- **Gated is retired from the menu.** It and Factory were both Fuzz Factory-style, so Factory (which adds Comp, germanium / silicon and Guitar Vol) is now the only one you can pick. Old sessions that use Gated still load and sound identical; the entry is just greyed out in the menu.
- **Always visible:** FUZZ ON, the style menu and AUTO LEVEL; then the style's own switches (GERMANIUM for Face, Bender and Factory; OP-AMP and the version menu for Wall; the model menu for Bender); then Gain, Tone, Level and Mix, plus the style's main knobs: Mids (Wall), Gate and Stab (Gated, Factory), Starve (Face, Bender), Low and High (Buzzsaw).
- **Behind ADV:** the Diodes menu and TONE BYPASS (Wall), BUFFER (Face, Bender, Factory), OCTAVE UP (every style), and the knobs Comp (Factory), Guitar Vol (Face, Bender, Factory), Drift and Hiss (every style).

## What is new in v56

**The Fuzz pedal has three new styles and a lot of new options.** The panel now has two pages, **MAIN** and **OPTIONS** (the open page is remembered with the session), and shows only the controls that do something for the style you picked. Old sessions and presets sound exactly the same (my 96-case comparison of the four old styles against v55, at both oversampling settings and with the Mix knob at 1 and 0.5, is bit-identical).

**Layout of the Fuzz panel**
- **Top:** FUZZ ON and the Style menu.
- **Second row:** the MAIN / OPTIONS tabs and AUTO LEVEL.
- **Always visible, switches:** GERMANIUM (the Face, Bender and Factory styles; lit = germanium, off = silicon), OP-AMP and the version menu (Wall), the model menu (Bender).
- **Always visible, knobs:** Gain, Tone, Level and Mix on every style, plus the style's own: Mids (Wall), Gate and Stab (Gated, Factory), Comp (Factory), Starve (Face, Bender), Low and High (Buzzsaw).
- **Behind ADV (v57), switches:** the Diodes menu and TONE BYPASS (Wall), BUFFER (Face, Bender, Factory), OCTAVE UP (every style).
- **Behind ADV (v57), knobs:** Drift and Hiss on every style, and Guitar Vol on Face, Bender and Factory.

**Face (round, germanium or silicon).** A two-transistor fuzz in the spirit of the classic round, woolly fuzz that reacts to your guitar. **Gain** is the fuzz amount, **Tone** a top-end roll-off, **Starve** a dying battery (less headroom, a gap at the zero crossing, so it gates and sputters), **Guitar Vol** is the volume knob on your guitar (turn it down and the fuzz cleans up) and **BUFFER** removes the pickup loading (brighter, tighter; off is the woolly, authentic sound). GERMANIUM: soft, warm, lopsided and leaky. Silicon: tighter, brighter, harder, more gain.

**Bender (raspy, 2 or 3 stage).** A raspy, cutting fuzz with a mid emphasis. A **Model** menu: Early (two transistors, thick and woolly), Classic (three transistors, raspy and cutting) and Late (three transistors, a tighter low end, more fizz, and Tone sweeps a wide range). Same GERMANIUM switch, Starve, Guitar Vol and BUFFER as the Face.

**Factory (gated, squealing).** A more complete take on the gated, unstable fuzz (the older Gated style is still there, unchanged). **Gate** starves it so notes cut off and sputter as they die, **Stab** lets it feed back and squeal (lower = more), **Comp** squashes the input so quiet notes lift and sustain, **Gain** is the drive, plus the GERMANIUM switch (soft and warm versus hard and bright), Guitar Vol and BUFFER.

**Wall (the Muff-style fuzz) got the biggest set of options.** With the defaults it is exactly the old sound.
- **OP-AMP:** the stages run out of swing and then clip hard, for a tighter, brighter, harder, more modern fuzz.
- **Version:** Standard (the old sound), Smooth (deeper mid scoop, smoother top), Gritty (thick, rougher, more low end), Mid-forward (more bite, tighter bass), Heavy (more gain, huge fuzzy lows).
- **Diodes:** Silicon (as before), LED (more headroom, louder, more open) or Germanium (clips early, softer, more compressed).
- **Mids:** 0.5 is the old sound; lower scoops further and higher fills the middle in.
- **TONE BYPASS:** the full, unfiltered fuzz.

**For every style (OPTIONS page).** **OCTAVE UP** puts a full-wave rectifier in front (a sputtering octave-up fuzz, best on single notes high on the neck). **Drift** slowly wanders the gain and bias (like a germanium fuzz warming up), and **Hiss** adds transistor hiss in front of the fuzz (louder as you turn the fuzz up).

**Where fuzz sits in the chain:** it was already movable. The ORDER button changes the order of the pre-amp effects, so fuzz before or after the wah is a matter of dragging it.

**Measured, not listened to.** Each new style's level was matched to the old ones with a plucked test signal (within about 4 dB of Wall, Tight and Buzzsaw at the default settings). The Face cleans up as Guitar Vol drops (down about 10 dB, with much less saturation, from 1 to 0.3), Starve takes about 5 dB off the level at 0.9, OCTAVE UP lowers the share of power below 150 Hz by about 2.5 dB, and the Wall options shift the 4 to 8 kHz band by roughly 0.5 to 2 dB (Op-Amp mode is the brightest). The Buffer switch is a subtle change on my test signal. I have not run it on a Mac or listened to it, so the character of each style (and how much the Ge/Si switch changes it) is my best guess and may need adjusting by ear. They are hand-built approximations, not circuit models.

**Four new factory presets:** Germanium Face Cleanup, Raspy Bender Rock, Factory Squeal and Op-Amp Heavy Wall.

## What is new in v55

**Four more amps, each with its own Voicing switch.** All four are also Amp B models, have their own colours in the window, and name both positions of the switch on the button. They are hand-tuned approximations built from measurements, not circuit models. Existing sessions are unchanged (my 48-case comparison with v54 is bit-identical, and the harmonizer comparison too).

**Low-Watt Combo (spongy breakup).** A small, low-power, two-stage combo. It breaks up early and softly, with mostly even harmonics, a lot of sag, loose lows, a honky low and upper mid and a rolled-off top. It cleans up when you play softly.
- **Soft** (switch off): spongy and honky, with edge-of-breakup grit at the middle of the knob. Measured distortion of a test tone: about 4% (Gain 0.2), 9% (0.5), 24% (0.8), almost all even-order at low gain.
- **Cranked** (switch on): the volume wide open. More gain, a thicker, spitty mid, much more sag and bloom, a rawer top. About 7%, 15%, 40%.

**Glass Clean (solid-state).** A stiff, wide-open solid-state style clean: huge headroom, no sag, symmetric clipping that only shows at the very top of the knob. A good base for chorus and ambient parts.
- **Glass** (off): bright, stiff and glassy. About 0.1%, 0.3%, 0.6% distortion, so it stays clean across the knob.
- **Warm** (on): rounder and darker, with a fuller low mid, a softer top and a little gentle compression. About 0.4%, 1%, 3%.

**Creamy Lead (tight, mid-hump).** A tight, creamy three-stage high-gain amp. It has the Modern Metal controls and Focus, like the Tight Metal amp.
- **Rhythm** (off): tight, punchy chunk, with a scooped low mid and a forward upper mid. Less fizz than Tight Metal Raw (at Gain 0.5 the 4 to 8 kHz band is about 2.6 dB lower).
- **Lead** (on): more gain, a big mid hump around 1.4 kHz, a fuller low end, more sag and a smooth, rounded top. About 28%, 37%, 45% distortion.

**Doom Stack (thick, loose).** A thick, loose, heavy amp for doom, stoner and sludge. No Modern Metal controls and no Focus (you want the looseness), but it does have all the amp realism controls.
- **Stoner** (off): fuzzy, saturated and fat, with a huge low end, a fat low mid, a dark top and heavy sag. About 19%, 33%, 37%.
- **Wall** (on): huge, deep and slow, with less preamp gain, very full bass and low mid, a dark smooth top and lots of power-amp sag. About 8%, 22%, 32%.

**Levels.** The trims are in the table under "Matching the amp volumes". At Amp Gain 0.5 all eight voicings sit within about 0.2 dB of each other in the model, and AUTO LEVEL has new rows for both switch positions (measured the same way as before, flat to within 0.01 dB across the Gain knob).

**Four new factory presets:** Low-Watt Edge of Breakup, Glass Chorus Clean, Creamy Lead With Delay and Doom Wall.

**Also fixed:** the preset browser's tag for the Acoustic and Classic Rock amps had been swapped; they now tag correctly.

**Limits.** I built and measured all of this but have not run it on a Mac or listened to it, so the voicings (especially how dark Doom Wall is and how bright Glass is) are my best guess from numbers. Glass Clean is about as bright as the Clean amp rather than brighter. If you want it more glassy, raise its Treble and Presence.

## What is new in v54

**Two new amps, each with its own Voicing switch.** Both are also Amp B models and have their own colours in the window (Boutique Lead: black with pale-gold trim and wheat grille cloth; Tight Metal: matte black with red trim). They are hand-tuned approximations built from measurements, not circuit models.
- **Boutique Lead (smooth, singing).** A smooth, round overdrive amp with lots of headroom at low gain that pushes into a singing, mid-forward lead tone. **Voicing off = Jazz:** clean, round, warm, a little less top end. **Voicing on = Rock:** pushed, more gain, more mid, long sustain.
- **Tight Metal (raw to modern).** A high-gain amp with a tight low end. **Voicing off = Raw:** bright, aggressive and tight, with a strong upper mid. **Voicing on = Modern:** more gain, tighter lows and a smoother top.
- Tight Metal also gets the **Modern Metal panel** (Tight, Mid, Sag, Lead, Chug Gate, Fizz, Depth) and the **Focus** knob for low tunings, the same as the Hot-Rod, Lead and High-Gain amps. Boutique Lead does not.
- The voicing button is labelled for what it does on each amp (VOICING: JAZZ / ROCK, VOICING: RAW / MODERN).
- **Levels.** Trims are in the table under "Matching the amp volumes". AUTO LEVEL has new rows for both amps, derived from my measurements so they land with the others (target -23.5 dB). At Amp Gain 0.5 they sit within about 0.4 dB of the other amps in the model.
- **Four new factory presets:** Boutique Jazz Clean, Boutique Rock Singing Lead, Tight Metal Raw Rhythm, Tight Metal Modern Lead.
- **Measured character** (harmonic distortion of a test tone, low to high gain): Boutique Jazz about 2% rising to about 19%; Boutique Rock about 6% to 34%; Tight Metal Raw about 18% to 32%; Tight Metal Modern about 20% to 26%. Boutique at low gain is mostly clean, as intended.
- **Existing sessions are unchanged.** The new amps are added at the end of the amp lists. My 48-case comparison with v53 is bit-identical, and the harmonizer comparison (8 cases) too.
- **Limits.** I have not run or listened to any of this; the voicings were set by measurement only, so expect to adjust Gain, Presence and the trims by ear. A modern boutique high-gain amp was later added as its own model in v61.

## What is new in v53

**The harmonizer can follow chords.** A new **Tracking** menu in the harmonizer panel has three choices:
- **Single note** is the old behaviour (about 14 ms of delay) and is still the default. Use it for lead lines.
- **Chords** follows up to 6 notes at once and moves each one by its own amount, so a C major chord harmonised a diatonic third up comes out as E, G, B, E (each note the right distance away), not one shifted copy of the whole chord. It adds about 85 ms of delay at 48 kHz (93 ms at 44.1 kHz), which is reported to your DAW. This one separates low notes best.
- **Chords (fast)** is the same with about 43 ms of delay. It cannot tell low notes that sit close together apart (the lowest strings in a chord blur), so it suits upper-string voicings.

A **"Hearing: …"** line under the menus shows the notes the harmonizer is hearing in Scale mode. If those are wrong, the harmony will be wrong. Glide is greyed out in Chords mode, because the pitch changes at once.

**How it works.** A note detector listens to the clean input. It scores every note from E1 to E6 by how many of its harmonics are present, takes the best, removes that note's harmonics and repeats, up to 6 times. A spectral pitch shifter then splits the sound into regions around each peak and shifts each region by the amount for its note. In Interval mode no detection is needed, since everything moves by the same amount, and the spectral shifter is just a better shifter for chords. A peak that belongs to no detected note (for example the first moments of a new chord) is shifted according to its own pitch.

**Measured** (synthetic plucked chords in my test harness, 14 common chord shapes):
- In Scale mode, how closely the output matches the ideal harmony (1.0 is perfect): **Chords 0.94**, the Single note engine 0.68. With perfect knowledge of the notes the figure would be 0.95, so the detector costs almost nothing. The weakest shapes were open G and open Em at 0.86 and 0.83, and a 7-string low chord at 0.81.
- In Interval mode (+5 semitones) on five chord shapes, the Chords window matched 0.98 on four of them (0.82 on a 7-string low chord, the hardest case); the Single note engine matched between 0.54 and 0.94.
- Chords (fast), the shorter window, matched only 0.36 to 0.70 on chords with low notes, and 0.99 on a high one. That is why Chords is the long window.
- Detector: on the 14 shapes it found 95% of the pitch classes (the note names without the octave) and 97% of what it reported was right. Individual notes are found less often (63%), mostly because a note an octave above another is not reported separately. That does not matter for the harmony: an octave double gets the same shift anyway. On random voicings of 1 to 6 strings (including odd clusters of neighbouring notes) pitch-class recall was 88% on 6 strings, 83% on 7 and 81% on 8. A single note was heard correctly every time.
- After a chord change, the new harmony was right in the first 40 ms window (match 0.88) and 0.95 or better after that.
- CPU on this Linux machine, 48 kHz, one core: harmonizer off 1.8%, Single note 2.9%, Chords 3.9%, Chords (fast) 4.4%.
- Compatibility: 48 of 48 state dumps identical to v52, and 8 of 8 harmonizer renders in Single note mode identical to v52.

**Honest limits.**
- I have not run or listened to this on a Mac. These numbers come from synthetic chords, not a real guitar. Build it and tell me what you hear.
- **Before the amp is where it works. After the cab with a high-gain amp it is much less reliable**: the distortion fills the spectrum with products that belong to no note. In my test (a C major chord through a high-gain amp) one of the four harmony notes came out far weaker than the others (0.002 against 0.007 to 0.013), where before a clean amp all four were within a factor of 1.6 of each other. Use before the amp for chords. The notes are still detected from the clean input in both positions.
- Notes that sit very close together in the low register blur (the low-chord cases above are the weakest ones), and two notes an octave apart are treated as one.
- Like any phase-vocoder shifter it softens pick attacks a little, and the harmony is made from a mono mix of left and right.
- Chords has about 6 times the delay of Single note (85 ms against 14 ms). For fast single-note lines, stay on Single note.
- The new parameter is called `hmTrack`. Presets from earlier versions load as Single note.

## What is new in v52

**Delays (Pre and Post) got a "Delay FX" panel.**
- **Duck**: echoes get quieter while you play and swell back when you stop. Duck Release sets how fast they return. Duck at 1 mutes the echoes completely while you play.
- **Mod** (depth and rate) wobbles the delay time, and **Diffuse** smears the repeats through four short allpass filters. In a test, diffusion cut the first echo peak from 0.166 to 0.091 with the same energy.
- **Hold**: *Freeze* stops new input and loops what is in the delay forever. *Infinite* keeps feeding back at a safe level, so you can play over it. A 10 s loud Infinite hold stays bounded.
- **Multi-tap** adds early echoes in 5 patterns (Half + full, Thirds, Quarters, Off-beat, Dotted cluster) with a Tap Level. Taps are not reversed in Reverse mode.

**Harmonizer (PITCH tab, next to the octaver).**
- Two voices, in Interval mode (fixed semitones) or Scale mode (pick a key and scale; the voice follows your notes diatonically).
- It can sit **before the amp** or **after the cab**. Pan only applies after the cab (stereo).
- Controls: Glide, Tone, Dry, and level and pan per voice.
- In Single note mode it tracks one note at a time, from about 38 Hz to 1 kHz, and chords confuse it (v53 adds a Chords mode for that). Tracker test: 9/9 notes found from 46 to 440 Hz.
- Latency is about 14 ms (672 samples) while it is on, and goes back to the previous value when it is off.

**Riff recorder (RIFF button, top right).**
- Records up to 60 s, stereo.
- **DI** records your dry guitar before the chain, so the playback goes through your current amp and effects (change the tone after recording). **Full tone** records the finished output.
- Loop, WAV export (24-bit) and WAV import. Riffs are not saved with the session.
- Tests: DI playback matches live playing, and Full tone playback is exact.

**Grey-out.** Controls that do nothing in the current setup are greyed out, and their tooltip starts with "Not in use right now: …" followed by the reason. Examples: a delay's tap level with Taps off, Mod FX controls the chosen type does not use, and the Amp B level when Amp B is off. Panels whose ON switch is off fade but stay usable.

**Layout.** There are now 11 tabs: PRE FX, BOOST, MOD, MOD FX, PITCH, AMP, CAB, SPEAKER, EQ, SCAPES, POST FX. The wow and octaver panels moved to the new PITCH tab. The chain strip has a "Harm." item. The amp tube, rectifier and bright row is always visible for both amps.

**Fixes.**
- **Chorus hiss**: the two noise sources used to cancel each other, so the hiss was not set where it was supposed to be. Each side now has its own noise. At maximum it measures −53 dBFS, and with the chorus off it is silent.
- **Amp B tubes**: the tube choice was wired, but the tubes differed in loudness and the menu was hidden. Level trims now match them (measured −24.9 / −25.5 / −26.4 dB), and the menu is visible.

**Checks.** 48 of 48 state dumps and 40 of 40 delay, chorus, Amp B and pitch renders are bit-identical to v51 for the paths that did not change. The old NAM and v51 tests still pass.

**Honest limits.** I have not run or listened to this on a Mac. Build it, and paste any compiler errors back to me.

## What is new in v51
**A boost with a parametric EQ, two multi-type modulation slots, reverse delay, a much wider reverb and granular section, string ranges for 7 and 8-string guitars, and NAM models in four places (each with its own loader).** Every new feature is off, or at a setting that changes nothing, by default. With the new features off, the sound is sample-for-sample identical to v50 on the same 48 test settings (checked: 0 differences), with one exception: v50 had a bug that affected NAM models when the overdrive was on (see the end of this section). I have not run v51 on a Mac, tried it in a DAW, or listened to it; I tested the audio code with a synthetic-signal harness and looked at screenshots of the pages. Please send me any build errors as pasted text.

### Boost (new BOOST page; in front of the overdrive and the amp)
- **What it is.** A pre-amp stage in this order: **Tighten** (a high-pass whose cut climbs from 40 Hz to 300 Hz as you turn it up, in three styles: Mid hump = low cut plus a mid hump at 720 Hz, Low cut = only the cut, Focus = low cut plus a push at 1.5 kHz) -> **Gain** (0 to 30 dB) -> **saturation** -> **Level**. It sits after the Lo-Fi Warble and before NAM Pedal 1, the overdrive and the amp. Off = an exact bypass.
- **Saturation modes.** *Clean* (no saturation, plain gain), *EP* (warm, bass-heavy, gently asymmetric, top rolled off: the character of a small tube tape-echo preamp used as a boost), *Console* (transformer-style: the lows saturate earlier than the highs, symmetrical and smooth, with a little silk on top: the character of a classic console preamp), *Tube* (an asymmetric triode-style stage), *Treble* (a treble booster: bass cut, upper-mid push, germanium-style asymmetric clipping). The **Saturation** knob sets how hard it is driven; the level rises with it (turn **Level** down if needed). These are my own hand-tuned voicings in the character named, not models of particular units (see MODELED_GEAR.md).
- **Boost EQ (parametric).** A low shelf, two bells with adjustable Q (100 Hz to 2 kHz, and 500 Hz to 8 kHz) and a high shelf, each +/-15 dB. **Before saturation** shapes what drives the saturation (cut lows to keep it tight, push mids to make it sing); **After saturation** shapes the finished sound. A band left at 0 dB costs nothing.
- **Checked.** All five modes give finite output at the extremes, off is bit-exact, and the EQ bands hit their set gain at their set frequency. **Limit:** the saturation runs at 2x oversampling with a gentle filter, so a very bright note driven hard can alias: about -45 dB for a 9 kHz tone at full saturation in the Console mode (the worst case I found). Treat it as a coloured boost, not a studio-grade distortion.

### Mod FX (new MOD FX page: two identical slots, A and B)
- **Types:** Flanger; Through-zero flanger (the dry signal is delayed 4 ms too, so the sweep passes through a deep null; this adds a fixed 4 ms delay while it is selected); Vibrato (pitch only); Uni-V (four swept all-pass stages with a lamp-like lag: half Mix is the phase-chorus sound, full Mix is vibrato); Rotary (a horn and a drum with their own speeds and inertia: Rate is the horn speed, about 0.8 Hz slow and 6.7 Hz fast, and Manual balances horn and drum); Auto-pan; Harmonic tremolo (lows and highs pulse in opposite directions).
- **Controls:** Position (**Before the amp** or **After the cab**), LFO shape (Sine, Triangle, Soft square), Rate with a Note menu (locks to the tempo, like the other LFOs), Depth, Feedback (flangers), Manual (flanger centre delay, or the rotary balance), Tone (rolls off the effect's top end) and Mix. Slot A starts as a Flanger and slot B as a Vibrato; both start switched off.
- **Notes.** The signal is mono until the amp, so before the amp the Auto-pan is a plain tremolo and the Rotary is mono; after the cab they are stereo. Both slots can run in either position. A slot switched off is a bit-exact bypass. All 14 type and position combinations give finite output.

### Post delay: Reverse (and ping-pong)
- The Post-Amp Delay has a new **Forward / Reverse** menu. In Reverse, each repeat is played backwards and swells up to the delay time (two overlapping windowed reads, so the repeats are continuous). The Warble control is ignored in Reverse. In my test the forward echo is silent before the delay time and loud after it, the reversed echo is the other way round.
- **Ping-pong was already there:** it is in the menu under the Mode menu ("Ping-Pong (left first)" / "(right first)", plus Dual). Combine it with Reverse for reversed echoes bouncing between the sides. The pre-amp delay is mono and has no Reverse.

### Reverb and scapes
- **Three new reverb types:** Soft Plate (dense, gently moving), Tumble (echoes that tumble over each other) and Abyss (huge, dark, drifting). I tuned their levels to sit within 8 dB of the Hall.
- **REVERB SHAPING panel (POST FX page, second row):** **Diffuse** (low = a lumpy, tumbling tail, high = a smooth wash; the middle is as voiced), **Space** (0.4x to 2.5x the room size), **Drag** (glides the tail's pitch up or down by up to 2 octaves while the knob moves, with a **Steps** menu: Smooth, Semitones, Fifths and octaves, Octaves; the middle is no change), **Mod** (slow movement: a three-voice chorus on the reverb output and a slight sway in the tail), **Bloom** (after a pause of about half a second, the reverb swells in behind the next note instead of starting at once; while you keep playing it stays fully open) and **Symphony** (a pitch-shifted layer in the tail: octave up, octave down, a fifth up or two octaves up). At their defaults they change nothing. Diffuse, Space and Drag apply to every type except Classic; Mod, Bloom and Symphony apply to all.
- **Granular cloud additions:** **Scale** (snaps the random grain pitches to octaves, fifths, pentatonics, major, minor or whole tone: it works from a Pitch Rand of 1 st or more, which sets how far up and down the grains go), **Shape** (grain fade: Smooth, Swell, Pluck, Flat), **Scan** (sweeps where the grains are taken from, so the texture keeps changing) and **Tone** (a low-pass on the cloud, in ADV). At their defaults the cloud is identical to v50.
- These were inspired by the reverb-and-texture pedals described in MODELED_GEAR.md; the controls are my own design on those ideas, not copies.
- **Checked:** every reverb type is finite at the extremes; Space and Drag move the tail; Bloom, after a pause, cut the reverb's level in the first 0.2 s of a note about tenfold (0.023 to 0.002); every granular scale and shape is finite and changes the sound. **Weak check:** Diffuse low versus high changed my tail-ripple measure only slightly (1.09 versus 1.07 dB of ripple), so judge that knob by ear and tell me if it does too little.

### Transpose, octave and pitch pedal for 7 and 8-string guitars
- A new **String range** menu on the Transpose panel (6-string, 7-string, 8-string) also sets the **Octaver** and the **Pitch pedal**. 6-string is the default and is bit-for-bit what v50 did. The 7-string and 8-string settings lengthen the shifter's analysis window and its search for splice points, so it can follow a low B (58 Hz) or low F# (40 Hz) string.
- **Measured** with a synthetic plucked note at the lowest string (F#1, 46 Hz), how much of the output energy lands on the intended pitch (1 = a clean shift): at +7 / +12 / -12 semitones the 6-string setting scored 0.02 / 0.04 / 0.06, the 7-string setting 0.99 / 0.99 / 0.00, and the 8-string setting 1.00 / 0.99 / 0.27. A standard low E (82 Hz) at +7 / -12 scored 0.53 / 0.17 on the old setting and 1.00 / 0.99 on the 8-string setting, so the longer window also helps a standard guitar's lowest string; the 6-string default stays as it was for the sake of its lower delay.
- **Cost:** the longer window adds delay, and the plugin reports it to your host. Measured with Transpose at +7 semitones: in the Fast mode 14.1 ms (6-string), 19.1 ms (7-string), 27.6 ms (8-string); the Smooth mode is 27.6 ms for all three. **Limit:** shifting the very lowest string of an 8-string down an octave is still weak (0.27 above), because a one-octave drop of 46 Hz needs a window longer than I allow.

### NAM models in four places, each with its own loader
- The **NAM tab** (AMP page) now has four strips, each with ON, **LOAD MODEL...**, **CLEAR**, MATCH LOUDNESS, Input and Output, and its own description line (which also shows why a file would not load): **Amp A** (as in v50, with the tone-stack knobs), **Amp B** (replaces Amp B's preamp and power stage; Amp B itself must be on; its tone knobs are on the strip), **Pedal 1** (a drive, boost or compressor capture in front of the overdrive and the amp) and **Pedal 2** (a capture after the cab, such as a reverb or a post-amp effect). The pedals have a **Mix** knob; the dry signal is delayed to match the model's own delay, so the blend stays in phase. Pedal 1 at Mix 0 is an exact bypass. Loading a model switches its slot on; each slot's file location is saved with the session and presets.
- **Pedal 2 STEREO** off: the stereo signal is summed to mono for one model whose output goes to both sides. On: a second copy of the model runs, one per side (about twice the CPU).
- The **AMP tab label** shows "NAM (ON)" while any slot is on; the signal strip shows NAM in place of the amp name when Amp A's capture is on.
- **Checked:** all four slots load and come back with the session; switching Amp B's capture on and off is click-free; every slot is finite with extreme settings. I did not measure CPU for four models at once; each standard-size WaveNet used about 11% of a core in the v50 measurement, so four of them could be heavy: use lite models or the mono pedal setting if it is.

### Fix to a v50 bug (NAM with the overdrive on)
In v50, when a NAM model was on, only the overdrive's **Volume** reached the model: its clipping and tone stage were skipped, even though the notes said the pedal still fed the model. In v51 the overdrive's clipping and tone run (at the oversampled rate, like for the built-in amp) before the model. A NAM preset with the overdrive on will therefore sound different from v50 (more like a real pedal into a capture). With the overdrive off, nothing changes.

### Chain strip
The strip under the tabs now includes Boost and a combined **Mod FX** stage (its light is on if either slot is on; a click switches both off, or switches slot A on), and the pages renumbered (10 tabs). NAM pedals are not on the strip (the Amp item says NAM when Amp A's capture is on).

### Checked
48 settings rendered with v50 and v51 are identical (0 differences). My test program passes: the NAM slots (load, round trip, bypass), Boost (bit-exact off, modes, EQ gains), the 14 Mod FX combinations, reverse versus forward echoes, reverb types and shaping, granular scales and shapes, and the 8-string range. Not measured: CPU of Boost, Mod FX and the new reverb options (the Symphony layer runs a pitch shifter, so it is the one to watch).

### Remaining gaps
- Looper and backing-track player; a graphic EQ in front of the amp (the Boost EQ is parametric with four bands); an FX-loop position for the delay; poly or per-string pitch effects; a library of NAM models with per-model calibration; newer NAM file variants; macro knobs and scenes; tone matching that also matches dynamics. (The earlier gaps table below is the v50 list.)
- The Reverse delay ignores the Warble control; Rotary and Auto-pan are mono before the amp; and NAM pedals are on a fixed place in the chain (not in the ORDER window).

## What was new in v50
**A .nam model player (NAM tab) and a Lo-Fi Warble pedal.** With NAM and Lo-Fi off, the sound is sample-for-sample identical to v49 (checked on the same 48 test settings). I have not run it on a Mac, tried it in a DAW, or listened to it.

- **NAM tab (AMP page, third tab).** Press LOAD MODEL and pick a `.nam` file (a neural capture of an amp or pedal chain made by someone else or by you). Loading a model switches NAM MODEL ON. While it is on, the capture replaces Amp A's preamp and power stage; the overdrive pedal still feeds it, and the tone stack (Bass, Low Mid, High Mid, Treble, shared with the AMP A tab) and the cab come after it. Amp B can still run beside it. **Input** sets how hard the model is driven (captures are made at a particular input level, so this changes gain and breakup), **Output** is the volume, and **MATCH LOUDNESS** evens out the volume between models using the loudness stored in the file (to -18 dB, the usual reference); switch it off for the model's raw level. The signal-flow strip shows NAM in place of the amp name. While NAM is on, the built-in amp's own gain, voicing, presence, power-stage and realism controls do nothing.
- **What it can play.** WaveNet captures (standard, lite, feather and nano sizes, with or without gating) and LSTM captures. Newer WaveNet variants, other architectures and container files are refused with a message that says why; a failed load keeps the model you had. Models are mono in, mono out.
- **Where the model lives.** The session and presets remember the file's location (like IR files), not a copy of it. If you move or delete the file, the session says it could not load it and falls back to the built-in amp. Keep your models in a stable folder.
- **Sample rate.** Models are trained at a fixed rate, nearly always 48 kHz. If your host runs at another rate, the plugin converts the rate in and out (windowed-sinc, about 1 ms of extra delay in total that is not reported to the host). At 48 kHz there is no conversion. While NAM is on, the amp's oversampling is not used (the model runs at its own rate).
- **CPU.** A standard-size WaveNet used about 11% of one core in my optimised test build on a PC, a lite or smaller model costs less, and an LSTM is cheap. A Mac with Apple silicon should be similar or better. Many small buffers (64 samples) cost a little more than large ones.
- **How I checked it.** I compared the output with the open-source reference engine on its example files (a small WaveNet, an LSTM, and a standard-size WaveNet) plus three synthetic models that use gating, ReLU and sigmoid: the differences were between -107 and -140 dB, and the result does not depend on the block size. The rate conversion keeps a 1 kHz tone within 0.01 dB and a 15 kHz tone within 0.01 dB at 44.1, 88.2 and 96 kHz. I could not test real captures of your own amps beyond the example models, so please tell me if one sounds wrong or will not load.
- **Timing.** When NAM is on, the delay the built-in amp's oversampler would have added is matched, so switching between them keeps the timing the host was told about.
- **Lo-Fi Warble (PRE FX page, just before the overdrive).** A worn, wobbly lo-fi pedal: a pitch warble (Rate, Depth, and a Sine / Triangle / Random shape), Drift (slow random wander plus a little fast flutter), Tone (dark to bright), Dirt (soft clipping plus bit and sample-rate crushing), Noise (hiss and crackle), Dropouts (random dips in level, like worn tape), Mix (100% = pure vibrato; lower blends in the clean signal for a chorus) and Level. It is a fixed stage (not in the ORDER list). The delay line behind it is 22 ms, so the wet signal is a little late when Mix is below 100%. Its defaults are a gentle worn-tape sound; set Depth, Drift, Dirt and Noise to 0 for a nearly transparent pass-through.
- **Checked:** the pedal is nearly transparent at 1 kHz when everything is at zero (within 0.1 dB), Mix at 0 passes the dry signal exactly, full Depth swings a 1 kHz tone by about 940 to 1060 Hz, Noise 1 sits around -29 dBFS, Dropouts reach deep dips, and extreme settings stay finite.

### Gap check for low-tuned modern metal, djent, ambient math-rock and clean technical tones
The rigs behind those styles (see MODELED_GEAR.md for the details and sources) are: a digital modeler running a tight high-gain amp into an IR cab on low-tuned 7- and 8-string guitars (modern metal and djent); a boost or overdrive in front to tighten the low end (modern metal); a gate; a class-A combo with compression, chorus/vibrato, delays, big reverbs, an octave pedal and a ten-band EQ (ambient math-rock); and a clean amp with light overdrive, compression, a mid boost, delay, reverb and chorus (clean technical).
- **Covered:** high-gain amps (Modern High-Gain, Saturated Lead, Hot-Rod, Classic Rock) with Tight, Chug Gate and Amp Focus for low tunings; the Tight overdrive style as a tightening boost; the gate; an IR loader (plus the new NAM player for captures of specific amps); a class-A amp (Vintage Chime); compressor; BBD chorus; digital, analog, tape and lo-fi delays; a reverb with shimmer and freeze; octave, pitch pedal and transposer; a nine-band post-cab EQ; and, new in v49 and v50, tone matching, power tube choices, and the Lo-Fi Warble.
- **Missing or thin:** an EQ or boost before the amp (a clean boost or mid boost, and a graphic EQ in front of the amp, which tight metal tones and the "slight mid boost" clean tones both use); a flanger (and other modulation such as a rotary or a classic vibrato switch separate from the chorus); a reverse delay and ping-pong or stereo delay modes; shimmer/octave reverbs with more control (the reverb has a shimmer tone but no separate pitch settings); an amp-style "FX loop" position for the delay; per-string or poly pitch effects; and 8-string-specific tuning of the pitch pedal and octaver (they work on the signal as it comes in, which is fine for low strings but not tuned for them).

### Remaining gaps compared with flagship amp software (not built)
| Gap | Why it matters | Effort |
|---|---|---|
| A pre-amp boost and EQ (clean boost, mid boost, graphic EQ in front of the amp) | Tight metal, and mid-boosted leads | Medium |
| Flanger, rotary, a vibrato mode; reverse delay and ping-pong delay | Ambient and modulation tones | Medium |
| NAM models on Amp B and in pedal slots; newer NAM variants; a model library and per-model input calibration | Using captures everywhere | Medium to high |
| Looper, and more practice tools (backing-track player) | Practice | Medium |
| More pedals and amps (a bass-friendly amp, a second clean) | Genre versatility | Medium |
| Tone matching that also matches dynamics and distortion | Closer matches | High |
| Smooth crossfade when the effect order changes, and drag-to-reorder on the strip | Polish | Low to medium |
| MIDI: per-mapping range, bank select, MIDI output, host program list | Controller use | Medium |
| A/B slots and undo history saved with the project | Convenience | Low |
| Non-WAV IRs and IR matching | Using your own recordings | High |
| Macro knobs / scenes that morph between settings | Live use | Medium |
| Stereo (dual-mono) amp input and a resizable preset browser | Flexibility | Low to medium |

## What is new in v49
**Power tube choices for both amps, a proper preamp tube label, and tone matching.** With the new power tube left on "As voiced" the sound is sample-for-sample identical to v48 (checked on the same 48 test settings). I have not run it on a Mac or tried it in a DAW, and I have not listened to any of it.

- **Power tube (ADV on the AMP A and AMP B tabs).** As voiced, EL34, 6L6, 6V6, EL84 or KT88. Each changes how soon the power stage breaks up (a drive offset or scale on top of the Power Drive knob), how much it sags, the tone-stack and presence balance, and a small level trim so they are about equally loud (measured on three amps, within about 1 dB of each other). Rough characters: EL34 crunchy upper mids; 6L6 tight, clean headroom, a little scooped; 6V6 soft, sagging, rolled-off top, breaks up early; EL84 bright, chimey, compressed; KT88 big and tight with the most headroom. These are hand-tuned approximations of general reputation, not models of specific tubes. Auto level is a table measured for "As voiced", so another tube can land about a dB off.
- **Preamp tube.** The old Tube setting (High gain / Medium / Low gain) was labelled as a power tube in its tooltip but always changed the preamp gain stages. It is now named for what it does: 12AX7 (high gain), 12AT7 (medium), 12AY7 (low gain). The sound and your saved settings are unchanged.
- **Tone matching (MATCH... button on the EQ panel).** Load a reference recording (up to the first 90 seconds are measured), press LISTEN and play for about ten seconds (chords and single notes), then APPLY TO EQ. The plugin measures the average spectrum of the reference and of your playing as it arrives at the Post EQ (third-octave bands, 50 Hz to 12.7 kHz, each analysis frame weighted equally and silence skipped), and fits the nine Post EQ faders by least squares so your tone moves toward the reference. It turns the EQ on. Strength scales the correction; the graph previews the result; RESET EQ flattens the faders. The reference is saved with the session and with presets. You can undo with Cmd+Z and fine-tune the faders by hand.
- **What tone matching cannot do.** It matches overall tonal balance only (the nine EQ bands, about 63 Hz to 8 kHz in practice; the 16 kHz fader is not measured). It does not copy distortion amount, dynamics, room, or the recording's own mastering EQ. A full-band mix as the reference pulls the result toward the whole mix (drums, vocals), so use guitar-forward material. Match after you set the amp and cab. The fit is limited to plus or minus 12 dB per fader; very different sounds will only get part of the way. It uses what reaches the Post EQ, so changing the amp, cab or any effect before the EQ after matching changes the result.

## What is new in v48
**Reorderable effects, a sturdier preset browser, A/B compare with level matching, undo/redo, a metronome and a tuner reference.** With every effect in its standard position the sound is sample-for-sample identical to v47 (checked on the same 48 test settings). I have not run it on a Mac or tried it in a DAW.

- **Effect order (ORDER button).** Two groups can be reordered: before the amp (fuzz, compressor, wah, phaser, chorus, pre-amp delay) and after the cab (Post EQ, textures, doubler, post-amp delay, reverb, widener). "Textures" is tremolo, wobble, the swell fade and the granular cloud together. Select an effect and press MOVE UP or MOVE DOWN; RESET ORDER restores the standard order. The signal-flow strip under the tabs follows the order. Each effect keeps its own settings and memory when moved. The noise pedal, transpose, pitch pedal, octaver, overdrive, amp and cab are fixed. The order is saved with sessions and presets, and a factory preset puts it back to standard. Changing the order while playing can click once, because the signal path changes at a block boundary.
- **Preset browser (BROWSE).** Factory and User tabs, a search box (names and tags) and a tag filter. Factory presets get tags worked out from their settings (style, amp type, fuzz, wah, pitch, modulation, ambient, delay). Your own presets can have tags: SAVE asks for a name and tags, and in the browser you can rename, re-tag and delete them. Damaged files in the folder are listed in red and cannot be loaded (they can be deleted). Renaming refuses to overwrite an existing preset. Loading a file that is not a valid preset shows a message instead of changing anything. **A fix:** in v47, loading a preset file also replaced your MIDI mappings; now presets never touch the MIDI mappings (sessions still keep them), as the v47 notes said.
- **A / B.** Press A or B to switch between two versions of your sound. The first time you press B it starts as a copy of A. The A/B menu copies the active slot to the other one and turns level matching on or off. **Level match:** while you play, the plugin keeps a slow average of how loud each slot is. When you switch, the new slot is trimmed (at most 12 dB either way) to be as loud as the old one was, and the trim stays fixed after that, so changing a knob is still audible. It only learns from what you actually play, so play the same part on A and on B; "Forget the measured levels" starts again. A/B slots live for the session only: they are not saved with the project.
- **Undo / redo (UNDO, REDO, Cmd+Z, Cmd+Shift+Z).** Every parameter value, the effect order and the A/B slot are covered, in up to 100 steps. Changes you make with the mouse or keyboard are grouped into steps (a whole knob drag is one step; so is loading a preset). Changes that come from MIDI or your DAW's automation are not counted as steps, so a wah pedal sweep does not fill the history. Not covered: the IR file paths, which cabs are loaded, MIDI mappings and window layout.
- **Metronome (METRO).** Click on every beat, with the first beat of the bar accented; 1 to 12 beats per bar; optional eighth, triplet or sixteenth subdivisions; Click, Wood or Beep sounds; volume. It is added after everything else, so the amp never touches it, and it uses the plugin's tempo (tempo box or tap tempo, or the DAW's tempo with HOST TEMPO on; when the DAW is playing it follows the DAW's beat position, assuming bars start at position 0). Metronome settings are kept when a factory preset loads.
- **Tuner.** The tuner already existed (open it with TUNER; it mutes the output). New: an adjustable reference pitch, A = 427 to 453 Hz.
- **Header.** The TEMPO caption is gone and the DELETE button moved into the browser to make room.

## What is new in v47
**MIDI: Program Change, controller mapping, pitch bend and Learn.** The plugin now accepts MIDI (a MIDI foot controller, an expression pedal, a keyboard's wheel, or a host automation lane of MIDI). The sound is unchanged: I rendered the same 48 test settings as v46 and they are sample-for-sample identical. I have not tried it with real MIDI hardware or in a DAW.

- **Program Change n loads factory preset n**, counting from 0 (so 0 is the first preset in the list, in the order of the Presets menu). The window follows (page, knobs and preset menu update). Numbers above the last preset are ignored. Loading happens off the audio thread, so there can be a brief gap between the message and the new sound; a short silence from the old preset's tail is possible.
- **Controllers (CC 0 to 127) and pitch bend** can each move one parameter, over its whole range (a CC of 0 is the minimum, 127 the maximum; pitch bend runs from fully down to fully up). Switches treat 64 and above as on. A mapping can be inverted.
- **Starting mappings:** CC 11 (expression pedal) moves the wah position and the pitch wheel moves the pitch pedal position, which gives you a pedal and a whammy-style wheel straight away. DEFAULTS in the MIDI window brings these back.
- **Learn:** right-click any knob and choose "MIDI learn", then move a controller; the next one that moves is assigned to that knob. Right-click also clears a mapping. For switches and menus, open the MIDI window (the MIDI button in the header), pick the parameter from the list, then press LEARN (or pick a CC number and press ASSIGN).
- **MIDI window:** channel filter (all channels by default), the list of mappings (with INVERT and REMOVE), and a line showing the last message received, which is handy for checking that your controller is arriving at all.
- The mappings and the channel are saved with the session, not with presets. Older sessions get the starting mappings.
- A parameter has one controller, and a controller drives one parameter. If a controller does not move the parameter you expect, check that another mapping does not already use that CC.
- The header has the MIDI button next to TUNER; it lights while a message is arriving and shows LEARN... while it waits.

Limits, so they are not a surprise: the plugin does not expose its presets to the host as programs (some hosts call "set program 0" while loading a project and would overwrite your sound, so I left that out), there is no MIDI output, no bank select, no note handling, and no separate low/high range per mapping yet.

## What is new in v46
**Lower CPU, no extra allocations, a smaller file, and a gap list. The sound is meant to be unchanged.** I checked the output against v45 with the same test signals (see "How this was checked" below); I have not run or listened to it on a Mac.

What changed:
1. **Parameter reads are cached.** The audio code asks for about 150 parameter values per block, and each one was a text search in a lookup table. They are now looked up once and then read directly (checked by instance, address and spelling, so a mismatch falls back to the old slow lookup). This helps most at small buffer sizes, where the per-block overhead is a bigger share.
2. **Idle effects stop working.** The Noise pedal's octave layer, the Clash layer and the Ring mod used to run every sample even when switched off. While idle they now only keep their pitch memory filled (so the effect still has your recent playing the moment you press it) and skip everything else. Turning them on, the fade-in and the fade-out are the same as before.
3. **Scratch buffers are reserved when playback starts**, so the audio thread no longer grows a buffer the first time it sees a larger block.
4. **Smaller file.** On Mac the linker now also drops local symbol names, and the editor (not time critical) is built for size instead of speed.
5. **Latency is unchanged, and already minimal where it can be:** Lowest latency / Low Latency mode is 0 samples, Balanced (2x) is 3 samples and Best (4x) is 4 samples, plus the look-ahead of the limiter only when it is on.

### Gaps compared with other flagship amp software (not built; a suggested order; MIDI was done in v47)
| Gap | Why it matters | Effort |
|---|---|---|
| Preset browser with tags and search, A/B compare, undo/redo | Faster tone finding | Medium |
| Metronome and a simple looper (the tuner already exists) | Practice tools that every flagship ships | Medium |
| Reorderable effects (pre/post amp, drag to move) | Many players want delay before drive, or EQ before amp | High |
| Tone-match / IR-matching and loading non-WAV IRs | Using your own recordings | High |
| Pre-amp EQ / boost slot and a few more pedals (a mid-boost overdrive, a clean boost) | Common tonal tricks | Low to medium |
| More amps (a second clean, a bass-friendly amp) | Genre versatility | Medium |
| Input and output level meters, and MIDI-learned macros | Gain staging and live control | Low to medium |

How this was checked: 48 settings rendered with v45 and v46 and compared sample by sample; noise pedal, Clash and Ring switched on and off in several orders; a randomised-settings run checking the output stays finite.

## What is new in v45
**A check of the high-gain amps for harshness and looseness, and the fixes.** I measured British Hot-Rod, Saturated Lead, Modern High-Gain and Classic Rock with test signals (palm-muted low power chords in E, a sustained lead note, a 2.9 kHz tone to look for aliasing, and noise to see the tone curve), through the default cab (the Modern Tight speaker, 4x12 closed, dynamic mic). These are measurements of the plugin's own output, not listening tests, and "harsh" and "loose" are my labels for the numbers below. What I found and changed:

1. **Aliasing (the main source of harsh, fizzy, "digital" top end).** With a 2.9 kHz tone at Amp Gain 0.75, about -32 dB of the output was inharmonic (aliased) energy in all three high-gain amps at the Best (4x) setting; Balanced (2x) was about -25 dB and Lowest latency (no oversampling) about -18 dB. The clipping stages were being fed everything the stage before had produced, including a lot of top-end. Fix: a gentle first-order low-pass at 6 kHz in front of gain stages 2 and 3, like the interstage roll-off of a real amp. Both Amp A and Amp B have it. Result at Gain 0.75: about -53 / -51 / -49 dB at Best (Hot-Rod, Lead, Modern), about -35 dB at Balanced, and about -20 dB with no oversampling; at Gain 1.0 about -36 dB at Best (was -30). Classic Rock was already clean and is unchanged. Also less fizz above 6 kHz: about 2 to 7 dB lower at 8 kHz and up.
2. **Modern High-Gain with Voicing off was the loosest and harshest.** Next to the other amps (same cab, Gain 0.75) it had about 5 to 10 dB more energy at 100 to 250 Hz than Hot-Rod, about 6 to 8 dB more at 3 to 5 kHz, and the most fizz. Changes: the three high-passes between the stages moved up (70 / 100 / 80 Hz to 95 / 130 / 100 Hz, the same as its Voicing-on setting), the bass shelf was cut from +3 dB to +0.5 dB, the pre-gain push moved down and lower (3.0 kHz +2 dB to 2.4 kHz +1 dB), the treble shelf and Presence range were reduced, and the sag was lowered a little. Result: energy under 70 Hz on a low-E chug went from -22.7 dB to -31.1 dB (relative to the total), close to its own Voicing-on setting (-32.0 dB), and the 3 to 4 kHz level came down by about 3 dB. On a down-tuned low-B chug the boxiness 100 to 250 Hz versus the mids was 15.7 dB; it is now 15.1 dB with Focus at 0 and 8.7 dB with Focus at 7 (it was 15.8 dB). It is still the bassiest of the group on purpose (about 4 to 7 dB above Hot-Rod at 100 to 200 Hz), but it is no longer flabby.
3. **Saturated Lead** was slightly loose at the very top of the Gain knob and with low tunings: its high-passes went from 85 / 125 / 90 Hz to 95 / 135 / 100 Hz and its bass shelf from +1.5 dB to +1 dB. On the low-B chug the energy under 70 Hz went from -31.7 dB to -35.6 dB with Focus at 7.
4. **British Hot-Rod and Classic Rock** were already tight and are unchanged apart from the shared 6 kHz roll-off (Classic Rock does not get it).

**What I did not change.**
- At Amp Gain 1.0 (the extra gain at the top of the knob) the Modern and Lead amps still sustain for 40 to 60 ms after a note is cut, by 3 to 7 percent of the note's energy, with the default closed 4x12 cab (the same chug with a 1x12 or an open-back cab gives about 1 percent). That is the sustain and compression of an extreme-gain amp plus the closed cab's low resonance, so I left it. If you want it tighter: raise Tight and Chug Gate on the Modern Metal panel, use Focus for the tuning, or use an open or 2x12 cab.
- The amps all have a 3 to 5 kHz bump of about +7 to +9 dB over the 0.5 to 2 kHz mean with the default cab, which is typical for a 4x12 with a dynamic mic. If your ears still find it harsh, take down Presence, or choose a speaker such as Smooth Lead.

**Levels.** The trims of the three amps were set again (see "Matching the amp volumes"). The AUTO LEVEL table was measured again with the new trims (flat to 0.01 dB across the Gain knob). Three factory presets (Wide Detune Doubler, Buzzsaw Rhythm, Technical Clarity) came out about 1 dB louder and were turned down to match (all 32 are back within about 0.3 dB of their old levels). The Clean, Vintage, Acoustic and Classic Rock amps are bit-for-bit identical to v44 (my 48-case comparison: all 24 cases that use those amps match; the 24 that use the three high-gain amps differ on purpose).

**Not tested on a Mac:** compiled and syntax-checked against JUCE 8.0.4 and measured offscreen on Linux, but I have not run or listened to it. Tone is subjective; if a change goes the wrong way for you, the numbers above are in the `voicings` table in `getVoicing` and the 6 kHz is `stageLpHz` (set to 0 to switch it off).

## What was new in v44
**Fuzz Mix knob.** The fuzz panel has a new Mix knob (0 = your clean guitar, 1 = all fuzz; the default is 1, which is the old sound exactly). Mixing in some clean signal keeps the pick attack and note definition under a heavy fuzz. The clean part is delayed by the same amount as the fuzz's oversampling, so the two are in phase and do not comb-filter. The fuzz Level knob only affects the fuzzed part (the clean part is always at unity), and with AUTO LEVEL on, the level follower works on the mixed result. The fuzz panel now shows four knobs per row.

Not tested on a Mac: compiled and syntax-checked against JUCE 8.0.4 and tested offscreen on Linux, but not run or listened to.

## What was new in v43
**Amp B has its own tab.** The AMP page now has two tabs, AMP A and AMP B. Amp B's panel is the same size and look as Amp A's: model menu, VOICING, AMP B ON, AUTO LEVEL and FED BY OVERDRIVE across the top, then Gain, Bass, Low Mid, High Mid, Treble, Presence, Tight, A Level and B Level, and the ADV controls (Bright, Tube type, Rectifier, Bias, Power Drive, Resonance, Sag). Under it is Amp B's own grille faceplate, in the colour of the model you chose. The tab reads "AMP B (ON)" while the parallel amp is running. The Acoustic model stays on the AMP A tab (no Amp B with it). The chosen tab is saved with the session. The window accent still follows Amp A.

**Classic Rock level.** I measured every amp's loudness across the Gain knob (the same plucked test signal as before) and moved the Classic Rock trims up by 1 dB (-1.0 dB, and -2.0 dB with Voicing on). It now sits within about 1 to 3 dB of the average of the other amps all the way across the knob. It cannot match everywhere with one trim, because it gets louder faster as you turn the gain up than the others do; the AUTO LEVEL switch below removes the rest. The five factory presets that use it were turned down about 0.8 dB so they stay at the same level as before.

**AUTO LEVEL.** A new switch on the modules where it makes sense. All are off by default, and with all of them off the sound is bit-for-bit the same as v42 (my 48-case comparison with v40 still matches exactly).
- **Amp A and Amp B (AUTO LEVEL on the amp row).** Holds the amp's loudness at the same value for every Gain knob setting, amp model and voicing, using a table I measured (the table adds no processing). In my test the level was flat to 0.01 dB across the Gain knob for every amp and voicing. It does not know about the Overdrive, the Modern Metal controls or a loaded IR, so those can still shift the level a little. B Level and A Level still work on top of it.
- **Compressor.** Adds the makeup gain for you from the Threshold, Ratio and Mix (a table). With it on, 0 dB on Output means "about as loud as with the compressor off"; in my tests within 0.4 dB over 12 Threshold / Ratio settings.
- **Overdrive.** Sets the overdrive's own loudness to match the guitar without it, from the style and Drive (a table measured through the Clean amp). The Volume knob then works relative to its -6 dB starting point. Leave it off when you use the overdrive as a boost into the amp, because the boost is the point.
- **Fuzz.** A slow level follower keeps the fuzz about as loud as the clean guitar going in, whatever Gain and style. The Level knob then works relative to its -9 dB starting point. In my tests within about 1 dB for all four styles. It holds its setting during silence.
- **Post EQ.** The same kind of level follower around the EQ, so boosting or cutting bands changes the tone and not the volume. In my test a +9 dB boost and a -9 dB cut came out within about 2 dB of unity (a +7 dB boost, and -4 dB for a cut, without it).
The level followers (Fuzz and Post EQ) read the level over about half a second and then move their gain smoothly, so a sudden change in how hard you play is levelled a moment later. They add no delay. I have not listened to any of the AUTO LEVEL options on a real guitar, so judge them by ear; the amp, compressor and overdrive ones are plain table lookups and the most predictable.

Where I left AUTO LEVEL off: the output limiter and the delays and reverbs (they have Mix controls, and levelling a wet effect would fight the mix), the pitch pedal and octaver (their Mix already sets the balance) and the cab (the cab blocks have their own Level knobs).

**Not tested on a Mac:** I compiled and syntax-checked the plugin against JUCE 8.0.4 and tested the DSP and the screens offscreen on Linux, but I have not run or listened to it.

## What was new in v42
**A new look and a clearer layout. The sound is unchanged** (the audio code was not touched; the processor file is identical to v41).
- **Signal-flow strip.** A new row under the tabs shows every stage in signal order, from IN to OUT: Transpose, Octave, Pitch, Noise, Fuzz, Comp, Wah, Phaser, Chorus, Pre-Delay, Drive, Amp, Cab, EQ, Delay, Reverb. Click a stage's light to switch it on or off without leaving the page you are on. Click its name to open its page. The page you are on is outlined. The Amp stage shows the current amp model.
- **The accent follows the amp.** The glow, lit knob arcs, lights and tab underline take the colour of the amp you chose (icy blue for Clean, amber for Vintage, coral for Hot-Rod, red for Lead, blue for Modern, orange for Classic Rock). Switching amp re-tints the whole window.
- **One clean panel style.** The brushed-metal rack units and coloured stompboxes (screws, rack ears, speckle) became one dark card with a hairline edge. Each pedal keeps its colour in the small bar beside its title. Knobs on those cards are flat dark discs inside a ring of light: the ring fills up to the value (from the centre for dB knobs that go both ways). Amp, cab and acoustic panels keep their tolex, cloth and wood looks.
- **Quieter controls.** Tabs are plain text with a glowing underline; buttons and menus are flatter; knob value boxes lost their outlines.
- **Background.** Soft beams of light from above, a faint speaker cone rising from the bottom-right corner and a vignette, all tinted by the accent.
- **Amp page.** With the Clean, Vintage and Classic Rock amps, the space under the controls now shows the amp's grille cloth with a big nameplate of the model. (The Modern Metal panel still takes that space for the Hot-Rod, Lead and High-Gain amps.)
- **Rows fixed.** In rows with two or three menus or buttons, the later ones used to get squeezed small (for example "Free (Hz)" cut to "Fr..."); each now gets its share of the row.
- **Header.** New wordmark with a small level-meter mark; the preset bar moved right a little to make room. The window is 1500 x 650 (was 616 tall), still resizable.

Not tested on a Mac: I rendered every page offscreen on Linux and checked the layout in those images, and the code compiles against JUCE 8.0.4, but I have not run it. Fonts differ on macOS, so check for text that is cut short.

## What was new in v41
**Factory presets (32).** The preset menu now starts with built-in presets grouped by style, then your own saved ones. They are tonal sketches in the spirit of modern and progressive metal: Djent & Modern Rhythm (4), Progressive Metal (6), Pitch & Octave (6), Groove & Sludge (4), Melodic & Extreme (3), Classic & Low Gain (3), Clean & Ambient (6). Choosing one resets the sound to defaults first, then applies the preset, so nothing from the previous tone is left behind. Your Input, Quality, Tempo, Limiter and Mono settings are kept. The < and > buttons step through the factory presets, then your own. Factory presets cannot be deleted; to change one, edit it and SAVE it under a new name. I level-matched them by measurement (a plucked test signal through each, to about -24 dB RMS; the clean and ambient ones are about 4 dB lower on purpose). I have not listened to any of them, so treat them as starting points.

**Classic Rock amp (crunch).** A seventh amp, added after Acoustic in the list (so old sessions are unchanged). An open, mid-forward, loose British-style rock amp. Voicing = Pushed (more gain and mid push). It is also an Amp B model. It does not have the Modern Metal controls.

**Open overdrive.** A third Overdrive style with Drive, Tone and a HIGH RANGE switch (high: more gain and a fuller drive; low: lighter and more dynamic). Transparent and Tight are unchanged.

**More amp realism (ADV on the AMP panel).** Bright switch, Preamp tube (12AX7 / 12AT7 / 12AY7, called High gain / Medium / Low gain before v49), Rectifier (Tube full sag / Silicon tight), Bias, Power Drive, Resonance and Sag. Amp B has the same set. All default to the old sound: with the defaults, my 48-case comparison with v40 was bit-for-bit identical.

**BASIC / ADVANCED panels.** Panels that had become crowded got an ADV button (top right). Off: the main controls only. On: everything. Hidden controls keep their settings and still work. The choice is saved with the session. Panels with ADV: Compressor, Wah, Phaser, Pre-amp delay, Post-amp delay, Reverb, Granular, Widener, Amp, Amp B, Modern Metal, each cab block, and the Pitch pedal. Note: a preset with an advanced control moved still sounds the same when ADV is off; you just cannot see that control until you turn ADV on.

**Pitch pedal (MOD page).** An expression-pedal-style pitch shifter you play with the Position knob (automate it from your DAW, or map a controller to it). Modes: Octave up, Two octaves up, Octave down, Two octaves down, Dive bomb, Bend (-12 to +12) and Custom (Heel and Toe, in semitones). Glide (ADV) smooths the movement. Mix 1 = shifted only. While on it adds delay (about 14 to 27 ms, following the Transpose quality menu), which is reported to the DAW. In my tests the shifted pitch was within about 0.3 Hz of the target in every mode. Position is not a MIDI input: adding MIDI input would change the plugin type, so for a pedal feel map a controller to the knob in your DAW.

**Octaver (MOD page).** Two shifted voices (two octaves down to two octaves up) blended with the dry guitar, plus a tone control. It also adds delay while on.

**SPEAKER page.** Per cab block: a five-band EQ (about 110 Hz shelf, 330 Hz, 1 kHz, 2.8 kHz, 6.5 kHz shelf), a resonance boost, and an all-pass Phase control for tuning how blocks combine. SPEAKER DYNAMICS (Amount, and a Tight / Vintage mode) adds level-dependent speaker compression and breakup; at 0 it is off, which is the old sound.

**Export cab as IR.** EXPORT WAV... saves the current cab blend as a stereo 24-bit 48 kHz WAV for use in other IR loaders. It includes each active block's cuts, EQ, resonance, phase, delay, polarity, level and pan, and the cab-wide filters. It does not include the speaker dynamics, room, the Amp A / B routing or Post EQ, and IR files are exported at their analysed (snapshot) length, so very long IRs are shortened.

**Not tested on a Mac:** I compiled and syntax-checked the plugin against JUCE 8.0.4 and tested the DSP and the screens offscreen on Linux, but I have not run or listened to it. The pitch pedal, octaver, realism controls and speaker dynamics are new DSP that I have only measured, not heard.

**MODELED_GEAR.md** now also lists what the new amp, overdrive, pitch pedal, octaver, speaker tools and factory presets are loosely inspired by (kept out of the plugin).

## What was new in v40
**Parallel Amp B.** The AMP page has a second amp, AMP B (PARALLEL), on the right. Switch it on with AMP B ON and both amps run side by side on the same guitar signal (the signal after the overdrive pedal):
- Amp B has its own model (the five electric amps), voicing switch, gain, tight, bass, low mid, high mid, treble and presence. It shares the overdrive pedal and Amp Focus with Amp A. FED BY OVERDRIVE turns the pedal off for Amp B alone.
- **A Level** and **B Level** balance the two amps (dB). A Level only does anything while Amp B is on.
- Each cab block has an **Amp** menu: Amp A, Amp B, or Amp A + B. So you can, say, send a tight high-gain amp to two cab blocks and a clean or chimey amp to the other two, and pan them apart. When a block is set to Amp A + B, the two amps are added at -3 dB each (so the sum is not much louder). With no cab block on a built-in model or IR (the plain fallback cab, or Cab Off), the two amps are simply added the same way.
- Amp B is off by default, and with it off nothing changes: the sound is identical to v39. It does not work with the Acoustic amp (the panel hides itself then).
- It costs about as much CPU again as Amp A. Both amps use the same oversampling setting, so the added delay is the same.
- Amp B has no chug gate, fizz tamer, depth or sag controls of its own (they stay with Amp A), and its mid push and sag are at the amp's own default.
- My tests: with Amp B set the same as Amp A and a cab block set to Amp B, the output matched Amp A to within rounding at all three quality settings, and with Amp B off the output was bit-for-bit the same as v39. I have not listened to it.

**MODELED_GEAR.md.** A new file next to this README lists the real gear each part is loosely inspired by (the "based on" names), kept out of the plugin itself. It says which names you gave me and which are my own reading. The plugin, its tooltips and this README still use generic names.

## What was new in v39
**Cabinets.** The built-in cab model has five new cabinet shapes (the old four are unchanged):
- **4x12 Straight** and **4x12 Angled**: a square front puts a little more upper-mid focus (about +1.4 dB near 2.3 kHz) in front of a mic on one speaker; an angled front has a little less of that and a touch more air and low warmth. The difference is real but small (about 2 dB around 2 kHz), so do not expect a dramatic change.
- **4x12 Open Back**: airy and loose, less bass.
- **Oversized 4x12 Straight (ported)** and **Oversized 4x12 Angled (ported)**: a large, deep, ported cab. The box resonance moves down (a ported peak near 62 Hz), the low end extends (about 6 dB more at 50 Hz than the normal closed 4x12) and rolls off steeply below that, and the boxy low mids around 300 Hz are cleaned up (about 3 dB). Good for downtuned, djent and metalcore rhythm; straight and angled differ in the same small way as above.

**Speakers and mics.** Six more speakers (Modern Scoop, Tight Ceramic, Dark Thump, Hi-Def Light Cone, Smooth Lead, Mid Forward) and six more mics (Dynamic Aggressive, Dynamic Punch, Dynamic Scoop, Ribbon Dark, Condenser Warm, and a dynamic + ribbon blend). They are hand-built approximations tuned by ear for modern high gain, with a range from dark and thick to bright and tight so they stay useful elsewhere. They are added at the end of each list, so saved sessions keep their choices.

**More gain.** The top of the Amp Gain knob now adds extra gain on top of the old range: below about 55% of the knob nothing changed, above it the first stage gains up to 1.4x (Clean), 1.6x (Vintage Chime), 2.2x (British Hot-Rod), 2.4x (Saturated Lead) or 2.8x (Modern High-Gain), and the later stages get less. The extra gain is mostly more saturation, sustain and compression. Expect more noise too, so use the gate and the chug gate.

**The panels follow the amp.** Each amp now has its own faceplate colours and styling (silver, cream, gold, chrome, or dark with white piping), its own knob caps and tick colours, and its cab blocks wear the matching speaker cloth. This is generic styling in the spirit of that kind of amp.

**Doubler** minimum time is now 5 ms (it was 8 ms). The random wander of the copies is scaled down for very short times.

**Tuner, meters and mono.** A TUNER button in the header opens a chromatic tuner (A = 440 Hz) over the page and mutes the output while it is open. The Output panel now has input and output peak meters (with clip lights; click to clear) and a MONO switch to check how the sound folds down.

**What is still missing** (ideas for later): two amps in parallel, A/B setting compare, MIDI / expression control, an IR morph or blend, a cab "ducking" delay, and a built-in metronome.

## What is new in v38
**Acoustic panel fixed.** The Acoustic / Piezo controls did not show because the panel was sized while it was still empty and was never laid out afterwards (the same kind of mistake as the Modern Metal panel in v34). Every panel is now laid out once all controls exist, and the acoustic panel is laid out again when it appears.

**Hardware look.** Panels now look like gear instead of frosted glass:
- **Pedals** (noise pedal, fuzz, compressor, overdrive, wah, phaser, chorus, both delays, transpose and the Scapes effects) are coloured stompbox enclosures with corner screws and a status jewel.
- **Rack units** (input and gate, cab controls, EQ, reverb, output, and the displays) are brushed-metal faceplates with rack ears, mounting slots and silk-screened titles.
- **Amp** panels (Amp, Modern Metal) are black tolex with gold piping and a brass nameplate. **Cab blocks** are speaker-cloth with cream piping; the **Acoustic** panel is wood with binding.
- **Knobs** follow the panel: machined aluminium in racks, black rubber in pedals, black with a brass cap on amps. The eleven ticks around each knob light up to the current value. The header carries a brass nameplate.
- Each panel's faceplate is drawn once into an image and reused, so the richer look costs almost nothing while you play.

**Mic position view** (CAB page, between the cab controls and the IR display). The left half shows the speaker from the front with one coloured marker per built-in cab block (centre of the cone to the edge = Position). The right half shows the speaker from the side with the selected block's mic at its Distance and Angle. Drag a marker (or the mic in the side view) to move it; drag the small dot on the back of the mic, or use the mouse wheel, to tilt it. Click A to D to choose a block. Blocks that are off or use an IR file show no mic. The knobs in each cab block still work and the view follows them.

## What is new in v37
**Compressor additions.** Three new knobs and a new saturation choice:
- **Bloom** makes the compressor squeeze harder and release more slowly, so notes duck and then swell back (up to 2.5x the gain reduction and 3x the release). The gain-reduction meter shows the real, scaled amount.
- **Smack** blends in a second, fast, heavy parallel compressor with the low end lifted. It adds weight and thickens quiet notes without flattening the pick attack.
- **Clarity** is a tilt EQ on the output (900 Hz pivot, plus or minus 4.5 dB). Right is brighter and tighter, left is warmer.
- **Transformer** (new in the Saturation menu) softly saturates only the low end (below about 160 Hz), more as the compression deepens.
All four are neutral at their defaults, so existing sessions sound the same.

**Post Delay stereo.** A new menu on the Post Delay: Stereo (same time, as before), Ping-Pong (left first), Ping-Pong (right first), and Dual (right offset). Ping-Pong feeds the input to one side and bounces the repeats across. Dual runs the right delay at 1x to 2x the left time (R Offset knob, shown only in Dual). The Spread knob (shown in any mode but Stereo) narrows the repeats toward the centre. Mono signals and the pre-amp delay behave as before. The Post Delay panel is wider and the Output panel slightly narrower.

## What is new in v36

### Shimmer reverb: more musical and more versatile
- **A better pitch shifter.** The shimmer now uses the same waveform-aligned shifter as the Transposer (with a 45 ms window), instead of the simple grainy one. Its splices stay in step with the notes, so the shimmer is smoother and less warbly.
- **Eight intervals** (the first three are unchanged): +12 (octave), +7 (fifth), +24 (two octaves), **-12** (octave down: a dark, deep, swelling shimmer), **+12 & +7** (octave and fifth together, two voices), **+12 & -12** (up and down together), **+5** (fourth) and **+19** (octave and fifth in one voice).
- **Mode** menu (new): *Feedback (blooming)* is the classic shimmer, where the reverb's tail is shifted and fed back so it climbs; *Input (no build-up)* shifts only your playing, once, into the reverb, for a clean octave pad that does not climb; *Both* does both.
- **Detune** knob (new, 0 to 50 cents, default 7): splits the shimmer into two voices a few cents either side of the pitch, each drifting slowly, which adds width and a choir-like softness and hides the shifter's grain. At 0 it is one steady voice. Because the default is 7, **older sessions will now sound a little wider and somewhat quieter in the shimmer than before (a few dB, since the two detuned voices partly cancel)**; raise Shimmer or set Detune to 0 to get the old balance.
- Mode and Detune appear only while SHIMMER is on. The reverb panel is wider to make room, and the knobs are in two rows of six.
- Tested standalone with the plugin's own reverb: all 8 intervals, all 3 modes and detune from 0 to 50 cents stay finite and bounded, with a 4 s Hall decay and also with a 30 s Cathedral decay, maximum Shimmer and 10 seconds of sustained input (peaks of about 1.6 inside the reverb in the worst case, tested with slightly higher voice levels than the final ones, before the Reverb Mix and the output stages). Feedback mode gets brighter as the tail climbs (the brightness measure rose from 0.09 to 0.14 over 8 s), and Input mode stays flat (0.03), which is the difference between the modes. I have not listened to it.
- Not changed: Shimmer amount (feedback, kept below 1), Tone, and the Low Cut and High Cut inside the loop.

### Acoustic amp page, checked and fixed
Reading the layout code for the acoustic model turned up three problems, now fixed:
- **The Bass, Low Mid, High Mid and Treble knobs were hidden** on the acoustic model, although the acoustic chain does use them as its tone controls. They now stay visible (Gain, Presence, Focus and Voicing, which the acoustic model ignores, are still hidden).
- **The Amp Quality menu is hidden** on the acoustic model, since there is no amp stage to oversample.
- The AMP panel is now sized to fit the model menu and four tone knobs, with the acoustic panel directly under it, and the Body menu no longer stretches across the whole window.
I could only check this by reading the code, not by seeing the window.

## What was new in v35

- **Transpose reworked for guitar.** The Transposer now has its own shifter instead of the simple one shared with the shimmer. Changes:
  - **Waveform-aligned splices.** Each time a read tap jumps back, it searches up to 7 ms either side for the position that best matches the waveform of the other tap (normalised cross-correlation), with a small penalty for large jumps. The two taps then stay in step, so the crossfade no longer cancels or beats. That beating was the warble you heard on sustained notes. 7 ms covers notes down to about 70 Hz (drop-tuned low strings are partly covered).
  - **Cubic interpolation** instead of linear, so the highs stay brighter.
  - **Smoothed shift amount** (about 15 ms), so turning Transpose or Fine glides without clicks.
  - It uses about 0.2 to 0.4 percent of one core.
- **Test results** (standalone, sustained tones with 2 to 4 harmonics, 28 ms window; "purity" is the share of the output energy sitting on harmonics of the expected pitch, 1.0 = perfect): the old shifter scored 0.0 to 0.3 on most low and middle notes (0.94 to 0.99 on the A3 tone with harmonics), and the new one scores 0.55 to 1.0 on everything except the case below, with more than half of the 20 cases above 0.9. The old one's best cases were the pure-harmonic A3 tone, where the two are about equal. The cases it handles worst: an E2 string (82 Hz) shifted down (0.34 at -12 st with the 28 ms window, 0.53 with 55 ms), because the new note's period (24 ms) is almost as long as the window. For low strings and down-shifts, use **Smooth** (or High Quality mode).
- Not changed: the Formant control (still an EQ-based approximation), the reported latency (half the window), and the shimmer, granular and noise-pedal octaves, which keep the simple shifter.
- It's a time-domain shifter, so chords, big shifts and very low notes will always be rougher than a spectral shifter. I have not heard it on a real guitar.

## What was new in v34

- **Fixed: the MODERN METAL panel never appeared** (on the British Hot-Rod, Saturated Lead and Modern High-Gain amps). The panel was sized only by an early call made while the window was still being built, before its controls had been added, so they were never laid out. It is now sized after its controls exist, and it also re-lays itself out whenever it is shown. Nothing else changed.
- Syntax-checked against the JUCE 8.0.4 headers; not linked or run.

## What was new in v33

- **Mode menu (header, left of the tempo controls): Normal / Low Latency / High Quality.** One switch for the whole plugin's speed-versus-quality trade-off. It overrides the individual quality menus, and Normal leaves them as you set them.

| | Normal | Low Latency | High Quality |
|---|---|---|---|
| Amp oversampling | Amp Quality menu (default 2x) | none (1x) | 4x |
| Fuzz oversampling | 4x if Amp Quality is Best, otherwise 2x | none | 4x |
| Transpose window | Transpose Mode menu (about 14 or 27 ms of delay) | 20 ms (about 10 ms of delay) | 55 ms (about 27 ms of delay) |
| Limiter look-ahead (if on) | 1 ms | none | 2 ms |

  Low Latency adds no delay from the amp or fuzz and the least CPU; high-gain tones can sound slightly fizzier and the transposer is a little less smooth. High Quality uses the most CPU and adds the most delay. All the delay is reported to your DAW.
- **Optional final limiter (OUTPUT panel, POST FX page).** Switch **LIMITER** on to put a brickwall limiter after the Output level. **Ceiling** (-12 to 0 dB, default -1) is the highest level it lets through; **Release** (10 to 1000 ms, default 150) is how fast it lets go. Ceiling and Release appear only while it is on. It is off by default.
  - It looks ahead (see the table): the gain is already down when a loud peak arrives and moves in smooth ramps, so it is much cleaner than a limiter with instant attack. In Low Latency mode there is no look-ahead, so the gain drops instantly, which is safe but harsher on large overshoots.
  - In a standalone test the output never went above the ceiling (stereo, noise and a 3x overload), and signal under the ceiling passed through unchanged apart from the look-ahead delay.
  - It limits **sample** peaks, not true (inter-sample) peaks, so a converter can still go slightly over; keep the ceiling at -1 dB or lower.
  - The look-ahead delay is reported to the DAW, and it changes when you switch the limiter on or off or change Mode.
- **Hover help for the items in every dropdown menu.** Hover a closed menu to see what the current item does (added under the menu's usual help), or open it and hover an item to see what that item does. This covers every menu: amp models, fuzz styles, compressor ratios, cab sources, cabinets, speakers and mics, reverb types, delay modes, note values and so on. The text lives in `itemTipTable()` near the top of `PluginEditor.cpp`. Showing a tooltip over an open menu uses a custom menu item; I could not test how it looks, so if the tooltip does not appear over the menu, the closed-menu help still works.
- **Syntax-checked against the real JUCE 8.0.4 headers** for the first time (both .cpp files, with extra warnings on, no errors). It has still not been linked or run.

## What was new in v32

- **Buzzsaw fuzz style** (new 4th entry in the Fuzz Style menu): a thick, sawing, scooped-mid distortion in the spirit of a classic two-band metal pedal. It is a very-high-gain stage (+26 to +56 dB, with the bandwidth shrinking as Gain rises) into slightly lopsided diode-style clipping, then a three-way split where the low and high bands are boosted and the mids are cut hard. **Tone** moves the band corners (the "colour" sweep: the scooped mid band moves up in pitch as Tone rises), and two new knobs, **Low** and **High**, appear only on this style. In a standalone test, Low and High each moved their band by about 7 to 8 dB, and the mids sat 3 to 9 dB under the lows and highs at Tone 0.5 to 1. It is a hand-built approximation of that kind of sound, not a circuit model, and I have not listened to it. Turn the amp Gain down and the Gate up, as this kind of fuzz is noisy. Old sessions are unaffected (the new style is added at the end of the menu).
- **Streamlined layout, nothing removed:**
  - The PRE FX page now follows the signal path: Input, Transpose, Noise pedal, Fuzz on the first row; Compressor and Overdrive on the second.
  - The MOD page holds Wah, Phaser, Chorus and, below them, the Pre-amp delay (it sits right after the chorus in the chain).
  - Each delay's Mode menu now shares the top row with the on switch and Note menu, so the knobs get one more row of height. Both delays are wider.
  - The tab tooltips are updated to match.
- **Lower CPU and latency:**
  - The Fuzz now runs at 2x on the Balanced and Fast amp qualities (4x only on Best), with lighter filters, so it should use roughly half the CPU of v31 (by operation count; I have not measured it) and add less delay. It follows the Amp Quality menu.
  - The per-sample `tanh`, `log10` and `exp` calls in the fuzz, compressor, delays, tape saturation and shimmer now use accurate fast versions (error around 1e-4, inaudible), which the compiler can vectorise.
  - The spectrum analyser's copy of the audio now runs only while the EQ page is showing, and the compressor meter only redraws when it moves.
- **Smaller build:** the unused audio codecs (FLAC, Ogg, MP3) are switched off and release builds drop unused code, in `CMakeLists.txt`. The README is a little longer, because it also holds the manual for every feature.
- Not compiled or listened to. The fast-math functions and the fuzz (all four styles at both oversampling settings) were tested standalone: finite, bounded and at similar levels. Fuzz latency from its own oversampler is a few samples and, as before, is not reported to the DAW.

## What was new in v31

- **Reverb Decay in seconds.** The reverb's 0 to 1 Size knob is now **Decay**, in seconds, from 0.1 to 30. For the Room, Hall, Plate, Ambient, Cathedral and Spring types it sets the time for the tail to fall by 60 dB, and longer decays also use longer delay lines (up to 2.5x), so the space grows as well. In a standalone test the measured decay was within about 20% of the setting (the damping makes it a little shorter) from 0.2 s to 30 s, and stayed stable. The **Classic** type has no decay time of its own, so the knob is mapped to its closest room size, which covers about 0.7 to 12 seconds (longer settings stay at its maximum). Old sessions are converted on load to the decay the old Size value gave.
- **Delay modes:** Digital, Analog, Tape and Lo-Fi on both delays (see "Delays").
- Not compiled or listened to. The delay modes and the long reverb decays were tested standalone for stability and for the repeats getting darker per mode; how they sound is untested.

## What was new in v30

### Transpose / formant (PRE FX page, first in the chain)
A pitch shifter at the very start of the chain (after the gate and high-pass), so the fuzz, overdrive and amp all play the shifted notes. Controls: **Transpose** (-24 to +24 semitones), **Fine** (+-50 cents), **Formant** (-12 to +12), **Mix** (1 = fully shifted; around 0.5 gives a harmony or detune), a **FORMANT LOCK** switch, and a **mode** menu (Fast, about 14 ms of delay, or Smooth, about 27 ms).
- Since v35 it uses its own guitar-tuned shifter (see "What is new in v35"): two crossfaded read taps whose splices are aligned to the waveform. It works best on single notes and gets rougher on chords. The dry signal is delayed by the same amount as the shifted one, so Mix blends them in time, and the delay is **reported to your DAW as latency while the effect is on** (it is 0 when off). Switching it on or off during playback makes the host update its delay compensation, which some hosts do with a small glitch.
- **The Formant control is an approximation.** True formant shifting needs a spectral-envelope analysis that I haven't built. Instead, Formant cuts the emphasis around two regions (700 Hz and 2.2 kHz) and re-adds it shifted up or down by the formant amount (up to 9 dB). With **Formant Lock** on, the plugin also applies the opposite of the transpose amount, so the overall tone stays close to the original as the pitch moves (otherwise shifting up sounds brighter and thinner and shifting down sounds darker and heavier). It is a tone-shaping aid, not a vocal-style formant engine.
- In a standalone test the shifted pitch is within about 2% of the target across -12 to +12 semitones, and the dry delay matches the reported latency exactly. How it sounds on a real guitar is untested.
- A bug fix along the way: the shared pitch shifter could read outside its buffer when shifting **down**. Nothing used it that way before (the shimmer and octave effects only shift up), but it is fixed.

### Modern Metal controls (AMP page)
A **MODERN METAL** panel appears under the amp for the **British Hot-Rod, Saturated Lead and Modern High-Gain** amps (it is hidden on the others). These are the sort of controls found on modern-metal amps; I'm not copying any particular amp's circuit or panel, and I don't know the internals of any real one. All values are my own, not tuned by ear.

| Control | What it does |
|---|---|
| LEAD BOOST | More gain in the first stages (+40% / +20%), +2 dB mid push, 30% more sag, level trimmed by -2.5 dB. For solos. |
| Tight | Raises the high-passes between the gain stages (up to about 2.8x higher). Palm mutes get tighter and clearer; the low end is thinner at full. |
| Depth | A low shelf at 90 Hz after the preamp, up to +6 dB, for thump. |
| Mid Freq | Slides the pre-gain mid push from 0.4x to 2.5x of the amp's own frequency (the middle is unchanged). |
| Mid Boost | Adds up to +8 dB to that pre-gain mid push. Because it is before the gain, it changes how the amp clips, not just the EQ. |
| Fizz Tamer | A low-pass after the preamp, from 12 kHz (off) down to 3.8 kHz. |
| Sag | Scales the power-amp compression (0 = stiff, 0.5 = the amp's own, 1 = double). |
| Chug Gate | A fast gate after the preamp, before the cab: instant attack, 12 ms release, with a threshold from -68 to -30 dBFS (0 = off). It cuts the hiss between palm mutes. If it chops notes, lower it. |

They all start at neutral, so existing sounds are unchanged. They combine with the Voicing switch, the Focus knob and the Tight overdrive. Tip for tight rhythm: Focus up to match your tuning, Tight 0.4 to 0.6, Chug Gate 0.3, Mid Boost 0.3, and the Tight overdrive in front with a low Drive.

- Not compiled or listened to. The transposer and chug gate were tested standalone; the rest of the new code has only been checked for consistency.

## What was new in v29

- **Attack is a knob.** On the Tight overdrive style, the 6-position Attack menu is now a knob (positions 1 to 6). The saved value is the same as before, so old sessions load unchanged.
- **Only the controls in use are shown.** Choosing the Acoustic amp hides the electric amp's Gain / Bass / Mids / Treble / Presence / Focus / Voicing and shows the acoustic panel (which is hidden otherwise). **Focus** only appears on the Saturated Lead and Modern High-Gain amps. The fuzz **Gate** and **Stab** knobs only appear on the Gated style. The shimmer menu and knobs only appear while Shimmer is on. The overdrive and cab-block panels already switched pages by style and source. The window checks every half second, so a preset or automation change updates the view too.
- **Acoustic is at the top of the amp menu**, then a separator, then the electric amps. (The numbers inside the plugin and in saved sessions did not change.)
- **Cabinet moved onto each cab block.** Each of the four blocks has its own Cabinet menu (1x12 Open to 4x12 Closed) under its speaker and mic, and the shared menu in the CAB section is gone. Old sessions copy their one shared cabinet setting to every block.
- **More reverb types** (see "Reverb" below): Room, Hall, Plate, Ambient, Cathedral and Spring, plus the original Classic.
- Not compiled or listened to. The new reverb engine was tested on its own (stable at every type and size, decay times as listed above, Freeze holds the tail); how it sounds is untested.

## What was new in v28

### Focus (was Tuning)
The AMP panel's **Tuning** knob is now **Focus**. It does exactly what it did (tightens the low end and trims bass and gain on the Saturated Lead and Modern High-Gain amps; see "Amp Focus"), only the name changed. Old sessions and presets load unchanged.

### Heavy voicing for the British Hot-Rod amp
The Voicing switch on the British Hot-Rod amp (it was "Fat") is now **Heavy**: a modern compact heavy-rhythm voicing with more gain and compression, a deep but controlled low end, a slight low-mid scoop, a forward upper mid for chunk, and a tamed top. See the Voicing table. It is my own interpretation of that kind of sound, not a model of a specific amp, and I have not tuned it by ear. The old "Fat" sound is gone; if you want it back, say so.

### Tempo sync on every time-based setting
Each of these has a **note menu** next to it ("Free" = use the knob; or 1/2, dotted 1/4, 1/4, 1/4 triplet, dotted 1/8, 1/8, 1/8 triplet, dotted 1/16, 1/16, 1/16 triplet). The tempo is the one in the header (TAP / BPM box / Host tempo). When a note is chosen, the matching knob greys out.

| Setting | What a note value does |
|---|---|
| Wah LFO Rate | one note length per sweep cycle |
| Phaser Rate | one note length per sweep cycle |
| Wobble Rate | one note length per cycle |
| Tremolo Rate, Pre-/Post-amp Delay Time | (already synced in v22/v27) |
| Granular **Size** | each grain lasts that note length (capped at 600 ms) |
| Granular **Density** | one grain per note length, so the cloud pulses with the tempo (kept between 1 and 80 grains per second) |
| Granular **Position** | grains read from that many beats back in the loop (capped at 3 s) |
| Swell Fade-In | each fade-in takes that note length (capped at 4 s) |
| Reverb Pre-Delay | pre-delay is that note length (capped at 250 ms) |

The chorus is not synced: its rate is a per-mode multiplier rather than a free rate in Hz.

### Brand names removed
Amp, speaker, mic, pedal and plugin names are gone from the interface, tooltips, parameter names and this README. Everything now has a generic name:
- Amps: Clean (sparkly), Vintage Chime (class-A), British Hot-Rod (tight), Saturated Lead (smooth), Modern High-Gain (scooped).
- Speakers: Modern Tight, Vintage Crunch, Rock Bite, Smooth Vintage, Chimey Alnico, Vintage American, Hi-Fi Full. Mics: Dynamic Bright, Dynamic Full, Dynamic Presence, Ribbon Smooth, Condenser Detailed, Flat.
- Overdrive: Transparent / Tight. Fuzz: Wall (sustained) / Tight (op-amp) / Gated (velcro). Compressor: 10:1 Soft, Limit, Tube-like / Tape-like / Both saturation.
- The "Gross" layer of the Noise pedal is now **CLASH** (scenes Bright / Dark / Full; knobs Drive and Blend). The widener's knobs are **Wide** and **Bass Lift**.
- Parameter IDs inside the plugin did not change, so old sessions and DAW automation keep working. Cab speaker/mic menus keep their positions, so saved settings select the same sounds under the new names.

### Lower CPU and lower latency
- **Amp Quality menu** (AMP panel, next to Voicing): **Best (4x)**, **Balanced (2x)** (the new default) or **Lowest latency (no oversampling)**. The amp's distortion runs oversampled to reduce aliasing; 4x is the smoothest but costs the most CPU and adds the most delay. 2x is roughly half the cost with a little more aliasing on high-gain settings, and "no oversampling" adds **zero latency** and costs the least, but high gain can sound fizzier. The delay the plugin reports to your DAW changes with the setting (it is told within a moment of changing it), so recordings stay lined up. **Because the default changed from 4x to 2x, the amp sounds very slightly different from v27; set Best (4x) to get the old sound.**
- **The amp and cab now run on one channel.** A guitar is mono, so the overdrive, amp, tone stack and each cab block's convolution used to do the same work twice (once per channel) and then the second copy was thrown away. That roughly halves their CPU. The result is identical.
- **Filters are only rebuilt when something changes.** Before, about 40 sets of filter coefficients were rebuilt on every audio block even if no knob moved. Now the plugin checks whether any parameter has moved and skips the rebuild if not.
- **Wah and phaser** do their expensive maths (exp, sin, pow, tan) every 4 to 8 samples instead of every sample. The sweeps are slow, so the sound is the same (in a side-by-side test against v27 the output differed by well under 1%).
- Smaller things: the amp's gain ramp no longer divides per sample, a few scratch buffers are sized in advance instead of on the audio thread, and the cab scratch buffers are mono.
- Not changed: the Fuzz pedal has its own small oversampler whose delay is not reported to the host (it is only active when Fuzz is on). The Doubler, Granular and Reverb are already cheap when off. The reverb's JUCE engine and the convolution run at their normal cost.
- I have not measured CPU in a real host; these are reductions in work, not benchmarks.

## What was new in v27
### A new look: "haze"
The whole interface has been redrawn (all in code, no image files): a deep violet-black window with soft coral, orchid and teal glows and a fine grain; frosted panels with a gradient edge and a lit dot in each title; **halo knobs** (a glowing orchid-to-coral value ring with a bright bead at the current value, on a glassy disc); **LED switches** (a small light that glows when a switch is on); pill-shaped menus and buttons; glowing faders; and tabs with a lit underline. To re-skin it, change the colours at the top of `PluginEditor.h` (the `ui` namespace: `accent`, `accent2`, `background` and so on) or the drawing code at the top of `PluginEditor.cpp`. I can't see it rendering from here, so spacing and legibility may need small tweaks after you run it. Tell me what looks off.

### SCAPES page: shoegaze and granular soundscapes
A new **SCAPES** tab sits between EQ and POST FX. These effects run after the Post EQ and before the post-amp delay and reverb (so you can wash them out with the big reverb and shimmer).
- **Granular Cloud.** The plugin keeps a 4-second loop of what you play and plays back lots of tiny overlapping pieces ("grains") from it, each with its own start point, pitch, direction and stereo position.
  - **Size** (10 to 600 ms): grain length. Short = grainy and glitchy; long = smooth, pad-like.
  - **Density** (1 to 80 grains per second): how many grains start each second. More = thicker cloud.
  - **Position** (20 ms to 3 s): how far back in the loop grains start. **Spray** scatters the start points up to 1.5 s further back.
  - **Pitch** (-24 to +24 semitones) and **Pitch Rand** (up to +-12 semitones, random per grain): +12 with a long Size gives a shimmer-like octave-up cloud; 0 with a little Pitch Rand gives a detuned wash.
  - **Reverse**: the chance that a grain plays backwards.
  - **Width**: how far grains are scattered left and right.
  - **Feedback**: feeds the cloud back into the loop, so it evolves and builds (kept below 0.9, with soft limiting).
  - **Mix**: a crossfade between your dry signal and the cloud.
  - **FREEZE**: stops recording, so the cloud keeps playing whatever was in the loop when you pressed it, after you stop playing. Turn on Position and Spray movement for an evolving drone.
  - The grain level is scaled down as density and size go up, to keep the loudness roughly steady.
- **Tremolo / Auto-pan.** Modes: both sides together (a classic tremolo) or the left and right alternate (auto-pan feel). Shapes: sine, triangle, soft square. **Rate** in Hz, or choose a **Note** value and the rate follows the tempo (the note length is one cycle; the Rate knob is then greyed out). **Depth**.
- **Wobble** (tremolo-arm vibrato). A short modulated delay used as a pitch vibrato, with a slow random **Drift**, like a loosely held tremolo arm. **Rate**, **Depth** (small values give a few cents of drift; high values are seasick), **Drift**, and **Mix** (1 = pure vibrato, lower = chorus-like).
- **Swell** (volume fade-in, "slow gear"). Finds the start of each note and fades it in over **Fade-In** (50 ms to 4 s). **Sensitivity** sets how easily it detects a new note. The detection listens to your signal before the amp; the fade is applied after the Post EQ. Strummed chords may re-trigger on each strum. Fast repeated notes (under about 120 ms apart) don't retrigger.

**Recipes (starting points, untested by ear):**
- *Shoegaze wall:* the Fuzz pedal in Wall mode, Chorus on, Wobble on (Depth 0.3, Drift 0.4), Tremolo on (Rate 4 Hz, Depth 0.3), big Reverb with Shimmer.
- *Granular pad:* Granular on, Size 300 ms, Density 20, Position 800 ms, Spray 0.5, Pitch +12, Width 1, Mix 0.6, Feedback 0.3; play a chord and press FREEZE.
- *Swell ambient:* Swell on (Fade-In 1500 ms), delay on, Reverb Mix up.
I tested the granular, tremolo, wobble and swell code on its own (stable, bounded, as expected). It is not a copy of any particular pedal or plugin.

### Hover help (tooltips)
Hover over any knob, switch, menu, tab or button for about half a second and a short explanation appears (what it does, its range or when it matters). The text for every parameter lives in one table near the top of `PluginEditor.cpp` (`tooltipTable`), so you can reword it. A parameter with no entry shows its own name.

### Doubler (POST FX page)
**DOUBLER** imitates double-tracking: the signal is turned into two copies, each delayed by a slightly different time and drifting slowly at random, darkened a little differently, and spread to the left and right. It is my own design. It sits after the textures and before the post-amp delay, and it needs a stereo output to do anything (it does nothing if the track is mono).
- **Time** (8 to 60 ms, default 22): the base delay. About 15 to 30 ms sounds like a second take; longer starts to sound like an echo.
- **Variation:** how much the two delays wander (up to about +-3 ms). 0 = a static, comb-like double.
- **Spread:** how far the copies are pushed left and right. 0 = both in the middle.
- **Mix:** how much of the doubled sound is blended in (the original is lowered slightly as you raise it).
Tested on its own: stable, and a mono input produces a clearly different left and right.

### Widener (POST FX page)
A stereo widener of the "make small speakers sound bigger" kind, with a **Wide** control and a **Bass Lift** control. I am not copying any particular unit; this is **my own design** in that spirit. It sits after the reverb, just before the output level, so the reverb is widened too. It needs a stereo signal (from the doubler, stereo cab blocks, or the reverb) since a perfectly mono signal has no left-right difference to widen.
- **WOW:** the left-right difference (side) signal has its lows removed, is smeared slightly in time and phase, has its low mids and highs lifted, and is made louder (up to about 3x). The centre signal is lowered slightly.
- **Space:** how much that smearing (a short delay plus an all-pass filter) is applied. Too much sounds unfocused.
- **Mode:** Classic, Desktop and Rack: three settings (delay time, all-pass frequency, how much lift) that I made up; they are not measured from the original variants.
- **Bass Lift:** adds harmonics of the low notes (from a rectified, filtered copy of the bass) and a little of the fundamental, so bass stays audible on small speakers. **Bass Freq** (50 to 200 Hz) picks the range. 0 = off. It makes the low end louder, so lower Level if needed.
- **Mono Bass** (20 to 300 Hz): the lows of the side signal are removed below this, so the bass stays centred. 20 Hz = off.
- **Mix** and **Level** (-12 to +6 dB).
Wide settings can sound hollow or disappear when summed to mono, so check in mono. Tested on its own with a stereo test signal: the side level goes up by about 7 dB at WOW 0.6, the centre drops about 1 dB, and output stays stable.

## What is new in v26: Spectrum display on the EQ page
Under the Post EQ there is now a **SPECTRUM** panel showing what the sound looks like right now (updates 30 times a second, only while the EQ page is open).
- **OUTPUT** (amber) = the final output after everything (including the output level). **PRE-EQ** (teal) = the signal just before the Post EQ (after the amp and cab), so you can see what the EQ adds or removes. **BOTH** draws the two on top of each other: move an EQ fader and watch the amber line leave the teal one.
- The horizontal axis is 20 Hz to 20 kHz on a log scale; the vertical axis is -96 to -6 dB (relative to full scale), with a line every 20 dB.
- **TILT** (on by default) tilts the display up by 4.5 dB per octave. A guitar signal falls steeply with frequency, so the tilt makes a typical sound look roughly flat, which is easier to judge by eye. Turn it off for the true spectrum.
- **HOLD** (on by default) draws a thin line that holds the recent peaks and slowly falls.
- The text line shows the peak and RMS level of the last ~85 ms in dBFS.
How it is made: the plugin keeps a copy of the last few thousand output samples (as a mono mix of the two channels) and the display runs a 4096-point FFT with a Hann window on it, taking the loudest part of each narrow band. It is a display only and doesn't change the sound. At low levels the picture is quite jumpy because there is little signal to average. It is not a calibrated analyser, and with very quiet signals the level readings below about -90 dB are not meaningful.

## What is new in v24
### Wah pedal (MOD page)
A new **MOD** tab holds the **Wah**, **Phaser** and **Chorus** (the phaser and chorus moved here from PRE FX). The signal order is: compressor, wah, phaser, chorus, then delay, overdrive and amp.
- **WAH ON** and **Mode:** *Manual (Position)* = you set the filter by hand (and can automate Position from your DAW, like a pedal); *Auto: Envelope* = playing harder opens the wah (auto-wah); *Auto: LFO* = it sweeps by itself.
- **Position** (0 = heel, back; 1 = toe, forward): the filter position in Manual mode; the resting position in Envelope mode; the centre of the sweep in LFO mode.
- **Heel** (200 to 900 Hz) and **Toe** (1.2 to 4 kHz): the lowest and highest centre frequency. Defaults 400 Hz and 2.2 kHz, close to a classic wah's range. The position sweeps between them smoothly on a log scale.
- **Resonance** (Q 1 to 12, default 5): how sharp the peak is. **Mix** (default 1 = fully wah; lower for a subtler blend).
- **Sensitivity** and **Decay** (Envelope mode): how strongly your playing level opens the wah, and how fast it closes again.
- **LFO Rate** (0.1 to 10 Hz) and **LFO Depth** (LFO mode): speed and width of the automatic sweep.
It is a state-variable band-pass filter, not a model of a particular pedal's circuit; it keeps the filter peak at about +4 dB. I tested that it is stable and peaks at the set frequency, but I haven't compared it with a real wah by ear.

### Focus knob
The old Tuning knob is now called **Focus** (same effect, same range; see "Amp Focus" below).

### Presets
The header has a preset menu with **< >** buttons to step through them, **SAVE** and **DELETE**.
- A preset is the whole plugin state: every knob, switch and menu, the cab block settings and the IR file paths (the IR files themselves are not copied, so keep them where they are).
- Presets are files named `Name.starteramp` in `Documents/StarterAmp/Presets`. SAVE asks for a name; saving a name that exists **replaces** it without a second prompt. Copy the files to share presets or back them up.
- Loading keeps you on the current tab. Presets from earlier versions load too (new controls get their defaults).
- Your DAW's own preset system and sessions still work separately.

## What is new in v23: Acoustic / piezo model
Choose **Acoustic (piezo to acoustic)** in the **Amp Model** menu (the last entry). The guitar amp stages and the guitar cab are bypassed and an acoustic-guitar chain takes their place, for a piezo or under-saddle pickup (or any DI'd acoustic signal). The ACOUSTIC / PIEZO panel is under the AMP panel on the AMP page.

What it does, in order:
1. **Piezo Fix:** cuts the nasal "quack" around 2.3 kHz and the glassy edge near 4.8 kHz, and fills in the hollow low mids around 300 Hz. A 70 Hz high-pass removes rumble. 0 = off.
2. **Preamp:** soft saturation (warmth), 0 = clean.
3. **Squeeze:** a gentle compressor (4 ms attack, 150 ms release; the threshold drops and the ratio rises from about 1.5:1 to 4.5:1 as you turn it up, with some make-up gain). 0 = off.
4. **Body menu and Body amount:** five resonant peaks that imitate an acoustic body's modes (air resonance around 100 Hz, top plate around 200 Hz, and so on). Choices: Dreadnought, Jumbo (bigger, lower), Parlor / OM (smaller, more midrange), Nylon classical (also softens the top end as Body goes up). 0 = no body.
5. **Sparkle** (high shelf at 5 kHz) and **Warmth** (low shelf at 200 Hz), each -6 to +6 dB.
6. **Room:** four short early reflections, crossing between the left and right channels for width.
7. **Level** (-12 to +12 dB). The Bass / Low Mid / High Mid / Treble knobs on the AMP panel still work as an EQ after this chain. Gain, Voicing, Focus and Presence do nothing in this mode.

Everything before the amp still works (gate, noise pedal, fuzz, compressor, phaser, chorus, pre-amp delay, overdrive), and everything after (Post EQ, delay, reverb) too. The overdrive, fuzz and noise pedals will sound odd on an acoustic, but they're there if you want them. The cab blocks are bypassed in this mode.

**How honest this is:** the body is a handful of hand-set peaks per guitar type, and the piezo fix is two cuts and a boost. They are my own rules of thumb, not measurements of any guitar or pickup, and I have not tuned them by ear. It is not a model of the real plugins' acoustic sections, which I know nothing about internally. Expect to adjust Piezo Fix, Body and Sparkle for your pickup. A real acoustic body responds differently to every string and note; this is a fixed EQ plus compression, not a physical model.

## What is new in v22
- **Tempo and note values for both delays.** A **TAP** button, a **BPM** box and a **HOST TEMPO** switch sit in the top-right of the window, on every page. Each delay has a **Note** menu (Free (ms), 1/2, dotted 1/4, 1/4, 1/4 triplet, dotted 1/8, 1/8, 1/8 triplet, dotted 1/16, 1/16, 1/16 triplet). With "Free (ms)" the Time knob works as before. With a note chosen, the time comes from the tempo and the Time knob is greyed out. Delay times are capped at about 2 seconds, so slow tempos with long notes get clamped.
  - TAP: tap at least twice; the tempo is the average of your last 5 taps (a pause over 2 seconds starts a new count). Tapping also turns HOST TEMPO off.
  - HOST TEMPO: follow the DAW's tempo if it reports one. In the standalone app there is no host, so the BPM box is used.
- **Input mode** (INPUT & GATE panel): Left, Right, or Mono (L + R). The chosen signal feeds both channels, so a guitar on one channel of an interface plays in both sides.
- **Up to four cab blocks (A to D)**, each with its own ON switch, and alignment that handles all of them (see below).
- **Focus knob on the AMP panel** for the Saturated Lead and Modern High-Gain amps (see "Amp Focus").


## Compatibility notes from older versions

- The editor window is resizable. It scales in proportion, so everything keeps its shape.
- `loadIR` loads on the UI thread. A very large file can briefly freeze the window.
- v22 notes: sessions from v21 or earlier load with the old A/B cab settings moved to blocks A and B (B only turns on if the old session used it). The old Mix A / B setting is ignored. New controls (tempo, input mode, tuning) start at their defaults.
- v21 adds controls, so saved state from v20 should load with the new ones at their defaults (new effects off, noise/delay/reverb switches on). Block A starts as a built-in cab model unless the saved session used an IR file, in which case it keeps that file.
- I have not linked or run the full plugin (the code was syntax-checked against JUCE 8.0.4 in v33); parts of the DSP were tested on their own (see the version sections). If the build shows errors, paste them to me.

- v41 notes: everything new defaults to neutral or off, so older sessions sound the same. The SPEAKER tab sits before EQ, so a session saved on the EQ, Scapes or Post FX tab reopens on the tab next to it. Classic Rock is added at the end of both amp lists (A and B).
- v40 notes: Amp B is off by default, so existing sessions sound the same. With Amp B on, the new Amp menu on each cab block defaults to Amp A + B, so set it to Amp A or Amp B per block if you want to separate the amps. Each block also grew a row, so the cab page is a little taller. A level and presence are not copied from Amp A: set Amp B yourself.
- v39 notes: new cabinet, speaker and mic choices are added at the end of their lists; existing sessions keep their choices. The top of the Amp Gain knob is louder and more saturated than before, so re-check high Gain settings. The tuner button does not save its state.
- v38 notes: the interface was restyled and the acoustic panel fixed; no audio or parameter changes, so existing sessions load and sound the same. I could not run the plugin here, so the exact look and the mic view dragging are untested on screen.
- v37 notes: the new compressor knobs and the delay stereo menu default to off/neutral, so old sessions are unchanged. Ping-Pong needs a stereo signal.
- v36 notes: the shimmer's new Detune knob defaults to 7 cents and Mode to Feedback, so existing sessions with shimmer on sound slightly different (see above). Everything else is unchanged.
- v33 notes: Mode defaults to Normal and the limiter to off, so older sessions sound the same.
- v32 notes: all new controls start neutral (the Fuzz Style default is unchanged), so older sessions sound the same apart from the fuzz's lighter oversampling and the tiny fast-math differences.
- v31 notes: the delay Mode menus default to Analog. Reverb Size values from older sessions are converted to seconds on load. All other controls are unchanged.
- v30 notes: all the new controls start neutral or off (Transpose off, Modern Metal at neutral), so older sessions sound the same.
- v29 notes: the Reverb Type menu defaults to Classic, so old sessions sound the same. The Attack value, the amp number and the cabinet setting migrate as described above. The amp menu is attached by hand (so Acoustic can be listed first), which is new code I have not been able to run.
- v28 notes: the new Amp Quality menu defaults to Balanced (2x); older sessions load with that value. The new tempo-note menus default to Free, so older sessions sound the same. Wah, phaser, the mono amp/cab path, change-detected filters and the oversampling switch were tested or reasoned about only in small standalone checks (the wah and phaser match v27 to within 1%); the full plugin has not been compiled or listened to, and the "Heavy" voicing and its level trim (-7.5 dB) are estimates. If the build shows errors, paste them to me.



