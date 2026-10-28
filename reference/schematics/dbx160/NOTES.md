# dbx 160 schematic (page "VII"), filed b58

File: `dbx160_schematic.png`. Read from the image only; values are as printed and I have not checked them against a second source.

Blocks seen:
- Input: balanced, RC filtering (2.2k / 1 nF) into OA3, a 4558 differential stage (20k / 10k, 1 % parts).
- Gain cell: dbx VCA module (type 200, "VCA M1") with symmetry trim (R27 50k) and a BC179 transistor "select with VCA". Followed by OA4 (4558) summing stage.
- Detector: dbx true RMS module (type 208, "RMS M2") with its own trim and symmetry (R34 50k), fed from OA3 via level-cal pot R63 (50k). It works in log (dB) terms.
- Sidechain: OA5 / OA6 set threshold (front panel, 20k) with above/below LEDs, and a front panel compress ratio control (R40, 20k with R41 7.5k, R43 50k "4:1 cal"), so the ratio is continuously variable rather than switched. Front panel gain control R80, 20k, +/-20 dB.
- Meter: OA4 and a meter driver with calibration pots (meter cal R56 50k, meter response R59 50k, zero R62 50k); a 3-gang switch picks gain change, output level or input level.
- Output stage: op-amp OA6 (301) driving a discrete class AB pair (BC341 / BC361) with KE4858 FETs (Q10 to Q13, four) in the feedback path; output on P3-1 / P3-2.
- Power: transformer, +24 V and -24 V raw, +/-15 V regulated (723 regulator with BC361 pass transistors, 4558 / 301 op-amps).

Why it matters for modelling: the detector is fed from the input stage (OA3), not from the output, which looks feed-forward, with an RMS module doing the level detection in dB terms. That is unlike the feedback FET, optical and variable-mu designs filed alongside it. This is my reading of the drawing, not stated on the page. No time constants or knee data appear on this page; the RMS module sets them.
