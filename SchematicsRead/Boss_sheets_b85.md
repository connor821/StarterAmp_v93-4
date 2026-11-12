# Boss sheets read in b85 (the 26 that were "filed, not read" in b83)

Each sheet was rasterized and looked at once; values below are what I could read at that size. Where a number is not given it was not legible, or the sheet has none. A sheet marked DSP has no analog circuit to model: the page only shows the control layout, the codec and the CPU.

## Flangers and chorus (bucket-brigade)
- **BF-2 (flanger).** Input follower (Q1), two uPC4558 sections as a pre-filter (47k / 100 pF, 4.7k with a 1S2473 diode pair in the feedback that limits the level going into the line), an **MN3207 1024-stage BBD** with its **MN3102** clock driver, two transistor follower post-filter stages (10k / 0.0082 uF and 10k / 0.0039 uF steps), a regeneration path from the post-filter back to the input through **Res (22 kB trimmer + 39k)**, a TL022 triangle LFO (Rate 250 kC, Depth 50 kB, Manual 50 kB), a transistor VCO feeding the MN3102 clock, and a two-transistor multivibrator for the LED. A 2SK30A in the output path is gated from the LED side; I could not follow its job.
- **HF-2 (Hi Band Flanger).** The same circuit as the BF-2 with an **MN3204 512-stage** line. The same clock range therefore gives **half the delay**: the sweep sits an octave higher. Same Res / Bias / Manual / Depth / Rate controls.
- **CE-3 (stereo chorus).** MN3207 (1024 stages) and MN3102, a TL022 LFO (Rate 100 kB, Depth 100 kB), JFET switches (Q2 to Q5, 2SK30A) around two M5218 sections, **two outputs (A and B)** and a **MODE I / II** slide switch. A 10k pot on the input (VR3). I read the two-output structure and the switch; I did not trace which side the inverted wet goes to.

## Phasers
- **PH-1r (1980).** Four all-pass stages: uPC4558 sections with **2SK30A-GR JFETs (Q2 to Q5, factory-selected, graded by pinch-off)** as the variable element, 10k series resistors and 0.01 uF caps, a TL022 LFO (Rate 1 MC, Depth 100 kB) into the gates, **Res 10 kB** from the last stage back to the input through 47k, a 1 kHz bias check (the sweep must reach 900 to 1100 Hz), and a multivibrator for the LED.
- **PH-2 (1984, "Super Phaser").** Two **IR3109 quad-OTA chips** (IC4, IC7: four OTA stages each, 470 pF and 68k per stage), an **HD14053** analogue switch (IC9) that routes them with the Mode switch (I / II), an NE571 half in front, 4558 / M5218 sections, JFET switching, **Res 10 kB, Depth 100 kB, Rate 250 kC and a 10 kB thermistor (RT-1)** on the control board. The routing of the switch (four stages versus eight) is my reading and is not confirmed.
- **PH-3 (DSP).** Stage, Res, Depth and Rate knobs and a mode switch; the audio goes through a codec and a DSP (33.8688 MHz clock, MN101C CPU). Nothing analogue.

## Wah and tremolo
- **AW-2 (auto wah, 1991).** 2SK184 input, a transistor band-pass filter built round a **700 mH** inductor (L1) with Q9 and a 2M5218 pair, an envelope detector (BA728 / IR9022 sections) and a low-frequency oscillator; controls Sens 50 kB, Rate 250 kC, Depth 50 kB, Manual 50 kB. JFET switches (Q4, Q5) pick the source.
- **AW-3 (DSP).** CPU, DSP and codec (UDA1340); control labels "Sens (Vowel 1)", "Manual (Vowel 2)", Decay, Mode. Nothing analogue.
- **TW-1 (touch wah).** Two uPC4558 sections, a 700 mH style filter, a **Peak (100k), Sens (50 kB) and a Drive up / down switch**; the adjustment note says to feed 350 Hz at -45 dBm and set the trimmer so the filter's top frequency lands between **1.3 kHz and 1.65 kHz**. JFET (2SK30A) switching near the output (I did not trace its job).
- **TR-2 (tremolo).** Input JFET (2SK1845), an M5216 inverting stage (22k / 10 pF), **two 2SK118Y JFETs (Q1 shunt, Q3 series) as the voltage-controlled resistors**, an LFO from M5281 sections (Rate 100 kB, **Wave 100 kA**, Depth 100 kB), output follower. The Wave knob blends the LFO from a triangle to a square (my reading of the two op-amp sections and the pot taper).

## Octave
- **OC-2 (1982).** A **BA634** (reference voltage, comparator, flip-flop, buffer) and a **uPD4013C** dual D flip-flop make the two octaves down (a divide by two for Oct 1 and a second divide for Oct 2); TL022 op-amps filter and gate; Direct 100 kB, Oct 1 100 kB, Oct 2 100 kB. The sheet's table (600 Hz in): output period 3.33 ms for Oct 1 (300 Hz) and 6.67 ms for Oct 2 (150 Hz); levels at -20 dBm in: Direct -21, Oct 1 -18, Oct 2 -24.5 dBm; at -50 dBm in: Direct -54.5, Oct 1 -48 (about 3 dB more for the octave than for the direct).
- **OC-3 (DSP).** Specs only: nominal input -20 dBu, 1 MOhm in, 1 kOhm out, 50 mA; controls Direct Level, Oct 1 Level, Control, Mode; two inputs (Guitar and Bass).

## Drives
- **SD-1 (1981).** uPC4558, Drive 1 MB in the feedback, Tone 20 kW, Level 10 kB, a diode pair in the feedback (one against two, drawn asymmetric), Q5 / Q6 buffers. Same family as the Screamer style; no values re-fitted.
- **OD-1 (1980).** uPC4558, Over Drive 1 MB, Level 10 kB, a 2SK30A in the level path, asymmetric diode pair; the LED-drive modification (0.01 uF, 220 pF) is a service fix only.
- **DS-1 (1st edition and service notes).** Same circuit as the one already built (2SC732 booster, TA7136P op-amp, Dist 100k class pot, 1S2473 clipping, Tone / Level pots). The sheet's waveform photos: 1 kHz in at -20 dBm (220 mV p-p) gives 1140 mV p-p or 440 mV p-p; at -40 dBm (22 mV p-p) gives 200 mV p-p or 440 mV p-p; the knob positions of those four photos are too small to read, so they were not used. The service-notes pages were rasterized but I compared only the first-edition circuit page.
- **DS-2.** An all-discrete design: input JFET (Q6, 2SK184), JFET / transistor gain stages (Q22, Q23, Q15, Q16, Q19), **a Turbo I / II switch**, **Dist 250 kD, Level 50 kA, Tone 100 kG**, a REMOTE jack, JFET switching (Q3, Q4, Q14, Q15, Q21) and two clipping diode blocks. Not modelled (no capture to fit and a long discrete chain).
- **OD-2.** All-discrete: JFET input (Q5), JFET source followers (Q13, Q14, Q10, Q11), diode networks (D9 to D11 with 47 pF), **Turbo switch**, **Drive 250 kA (dual), Level 100 kB, Tone 10 kB**, 8.2 V and 5.6 V rails. Not modelled.
- **OD-3 (a redrawn copy, not Boss's own sheet).** The file is a third-party redraw ("drawn by J. Luja 2002, Rev 2"): 2SK184 JFET front end, 1N914 pairs, a 2SA1048 stage, Drive 100 kA, Tone 10 kB, Level 100 kB, two M5218 sections for tone. Treated as unverified.
- **BD-2.** Read against the model already built: the 1.5k / 0.15 uF shunt, the dual 250 kA Gain, 0.018 uF tone caps with a 10 kB Tone and 100 kB, silicon diode clipping between the stages. **It matches the Blues Driver style as built; no change.**
- **CS-2 (sustainer).** BA662A OTA (IC1), input follower (2SK30A / 2SC732), Sustain 1 MC, Attack 150 KC, Level 50 KA, an envelope rectifier (Q8, Q9) and a 2SK30A gain-control FET. **The sheet gives a measured output table (Level at max):** 1 kHz at -50 dBm in gives about -12 dBm out; -40 in gives -8.5 (Sustain max) or -33 (Sustain min); 0 dBm in gives about -8 (slight distortion); 100 Hz at 0 dBm in gives -4.5 (Attack max) or -10.5 (Attack min), with the two waveforms drawn. **Used in b85** (see CIRCUITS.md).

## Delay
- **DM-1 (1978, rack).** Transformer-powered (117 V / 220 V), uPC4558, an R5101 bucket-brigade chip, TC4013 / TC4069 / TC4016 CMOS (clock and switches), about six transistor filter stages per side, controls Input Level, Delay Time 50 kB, Intensity, Balance. The board is dense; I read the parts list and the block structure, not the values.
- **DM-3.** The DM-2's family: **MN3205 (4096 stages)** with an MN3102, a **uPC1571C compander** (IC2), three-section filters before and after the line (10k / 330 pF, 0.039 uF), JFET input and output switching, Repeat Rate 50 kB, Intensity 50 kB, Echo 50 kB. Like the DM-2 it has a compander; the plug-in's DM-2 style has none, and that is still waiting on your answer.

## Not covered
The page set for DS-1's service notes (6 pages), OD-1's first three pages, DS-1 and OD-1 parts lists and the OC-3 service-notes pages other than the specification were rasterized and skimmed, not studied. BF-3T and AW-3 are DSP boards (text extraction gives the codec and CPU labels only).

## Update (b86)
BF-2, HF-2 and OC-2 are now built: see CHANGELOG b86 and CIRCUITS.md. Estimates are labelled there (delay ranges, post-filter corner, divider filter corner, hysteresis, envelope speeds).
