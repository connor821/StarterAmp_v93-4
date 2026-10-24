# Output transformer data (Hammond 1750 series)

Source: the three Hammond datasheets you supplied (reference/datasheets). Measured by Hammond at 1 kHz, 1 V; "typical". These are replacement transformers
built to match original amps, so they are a reasonable stand-in for the originals, not the originals themselves.

| | 1750KA (Fender-style 50 W) | 1750ORF (Orange Rocker 30-style, 35 W) | 1750D (Marshall-style 100 W) |
|---|---|---|---|
| Primary impedance (plate to plate) | 4150 ohm | 6600 ohm | 2350 ohm |
| Secondary | 2 ohm | 8 and 16 ohm | 4, 8, 16 ohm |
| Primary DC resistance | 45.5 + 49.1 ohm (two halves) | 68.4 + 72.2 ohm | 67.9 ohm (whole primary) |
| Secondary DC resistance | 0.14 ohm | 0.60 ohm | 0.30 to 0.86 ohm (by tap) |
| Primary inductance | 11.3 H | 10.3 H | 10.2 H |
| Leakage inductance | 5.16 mH | 18.25 mH | 5.66 mH |
| Stated response (0 / -1 dB at 1 kHz) | 70 Hz to 15 kHz | 70 Hz to 12 kHz | 70 Hz to 15 kHz |
| Distortion | under 1 % at 70 Hz | under 1 % at 70 Hz | under 1 % at 70 Hz |
| Feedback winding | none | none | yes, 0.025 ohm |

## What these numbers do to the sound (my calculation, not a measurement)

- **Low end:** primary inductance against the load the tubes see. With the amp's own source impedance of 4 to 8 kohm, the -3 dB point lands around 23 to 56 Hz. That is below a guitar's lowest string, so the transformer's bass rolloff is real but not the thing you hear. What you hear is saturation: a small core (35 W) runs out of flux sooner at low frequency and high level than a big one (100 W). The datasheets give no core size or saturation curve, so that part is not in the numbers.
- **High end:** leakage inductance against the reflected load puts a pole at about 58 to 128 kHz. Far above the audio band, so the top end is set by winding capacitance and the speaker, not by these figures. The 1750ORF's leakage is 3x the others (18 mH), so it is the one that would roll the top off earliest.
- **Copper loss:** primary resistance is 1 to 3 percent of the reflected load, so the loss is about 0.2 dB. Not audible as level; it matters only as a small extra source resistance.

So: this data supports the current power-section settings (a low-end saturation amount per amp, feedback amount, supply droop) but would not by itself change the sound. Adding inputs for winding resistance and leakage would change the output by well under 1 dB. The thing worth having and not found here is a flux-saturation figure (core cross-section, or a measured level-versus-distortion curve at 40 to 80 Hz) for each transformer.

Not used in the model yet (see CHANGELOG b22).

## Soldano SLO-100 output transformer (replacement parts, received b27)
Published specs, not measurements of an original DeYoung transformer (keep the two kinds of record separate).
| Parameter | Value | Source |
|---|---|---|
| Primary impedance | 2,200 ohm | Sowter 1401, Tube-Town TT-Slo100 |
| Primary inductance | 18.5 H | Tube-Town |
| Secondary taps | 4 / 8 / 16 ohm | both |
| Rated power | 100 W | both |
| Response | 20 Hz to 100 kHz; overload point 100 W at 50 Hz | Sowter |
| Winding resistances, leakage inductance, interwinding capacitance, saturation curve | not published | needs measurement |
Power transformer (Tube Amp Doctor replacement): 360-0-360 V at 450 mA, 60 V bias tap, heaters 6.3 V 7.5 A and 6.3 V 3 A, EI114 core.
**What it means (my calculation):** 18.5 H against the 2.2 kohm primary puts the low-frequency corner at about 19 Hz (about 38 Hz if the tubes see 4.4 kohm), so the 18.5 H would not by itself change the sound: only the saturation curve could, and it is not published. Not used in the model. Consistent with the earlier finding that the useful missing number is low-frequency core saturation.
