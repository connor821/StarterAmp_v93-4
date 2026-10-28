# DIY 1176 build guide (Mason Audio) - condensed notes

Source: https://www.masonaudio.org/diy/comp1176 (Matteo Masi). Filed as a condensed summary, not a copy: the container cannot reach that site or the Gyraf/Mnats/AXT links, so the page was read through the web fetch tool only. Linked files (Gyraf Rev. F schematic gif, BOM xls, calibration PDFs) are NOT filed; send them if wanted.
The user-supplied schematic here is `UREI_1176LN_RevK_schematic.png` (UREI 1176LN, effective for S/N suffix "K", static DC voltages added 23-10-02).

## Circuit, as the guide describes it
- Three parts: input (pot attenuates before the input transformer so the GR FET is not overdriven), gain reduction, output.
- Gain reduction: one BF245A used as a variable resistor. Feedback-style compression: the control signal is sensed after the GR stage and applied to the FET gate; more gate voltage = more reduction. (The schematic also shows a second BF245A, Q10, as the GR meter driver.)
- Output: push-pull class AB driving the output transformer, which sits inside the feedback loop (Lundahl LL5402 on the schematic).
- Metering: second BF245A tracks the first; SIFAM AL29WF VU meter.
- Rails: +30 V and -10 V; 7824 regulator (VR1) for the positive rail.
- Revisions A/B: 1108-style class A stages, some distortion. C/D/E: blackface, "LN" low-noise changes. F: class AB push-pull stage plus output transformer, op-amp meter. G: input transformer replaced by IC differential amp. The FET compression circuit stayed largely the same across revisions. F and G measure lowest distortion.

## Measured ratios (author's Rev. F build)
Set 4 / 8 / 12 / 20 -> measured 5.1 / 8.2 / 11.5 / 20.5 (delta in / delta out at 0 dBu). Useful as a target for a modelled ratio set.

## Calibration (author's procedure, abbreviated)
Warm up 10 min. Q-bias: bypass GR (attack off or short pin 22 to ground), 0 dBu at 1 kHz, set in/out to +1 VU, turn bias trimmer from fully CCW until 0 VU. GR meter zero with no input. Tracking: ratio 20, release 12 o'clock, 0 VU shorted vs -10 VU unshorted, repeat until exactly 10 dB. Distortion trim optional.

## Build details (not relevant to the plug-in)
FET matching (Vds vs Vgs, 4 of 9 devices matched), etching steps, lamp resistor 180 ohm 5 W, star ground at XLR pin 1, Lundahl vs OEP transformers (OEP darker, Lundahl clearer highs).

## Linked sources (unfetched)
Gyraf Rev. F schematic https://www.gyraf.dk/gy_pd/1176/1176sch.gif ; original manual https://thehistoryofrecording.com/Manuals/UREI/Urei%201176LN%2001.pdf ; Mnats PCB https://mnats.net/1176.html ; AXT ratio procedure https://axtsystems.com/axtsystems/proj_1176_ratios.php ; BOM https://www.masonaudio.org/ma_files/1176_bom.xls
