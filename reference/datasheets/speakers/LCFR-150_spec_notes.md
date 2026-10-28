# LCFR-150 guitar speaker (Fane-built, sold with the Prometheus amp) - spec text as supplied

Source: product page text pasted by Connor (Oct 2026). No datasheet PDF or frequency plot; IRs by John Browne (the three SM57
close-mic IRs already used to fit "Full-Range Open", v94).

Claims: low end "extends to 55 Hz in the right cabinet", resonance peak at 65 Hz, paper cone and dust cap without doping
(shimmering, not piercing highs), "complex mid voicing with lots of growl". Frequency range 55 Hz - 5 kHz. 150 W.

| Item | Value |
|---|---|
| Nominal diameter / cut-out | 12 in / 286 mm |
| Impedance | 8 ohm (Re 6.8 ohm) |
| Sensitivity | 99.43 dB |
| Magnet | ceramic, 1.1 kg; steel chassis; copper voice coil; paper cone |
| Nett weight | 4.3 kg |

Thiele-Small: Fs 65 Hz; Qms 7.6; Qes 0.451; Qts 0.426; Vas 92 L; Vd 0.224 L; Sd 297.2 cm2; Cms 0.262 mm/N; BL 12 T/m;
Mms 23 g; Xmax 4.5 mm; efficiency 5.352 %.

Notes for the model:
- Fs 65 Hz is the free-air resonance. In the IRs the open-back cab and close mic move the visible low-end shape (the fitted
  speaker has the 94 Hz high-pass and a 159 Hz bump), so the 65 Hz figure is not applied directly to the acoustic curve.
- Useful for the electrical-load / impedance model (Re, Fs, Qes, Qms, BL, Mms are all given: a full impedance curve can be
  computed). 5 kHz range matches the fitted lowpass (~4.8 kHz).
- Qes 0.451 / Qms 7.6 give a free-air impedance peak of about Re*(1+Qms/Qes) = 6.8 * 17.9 = ~122 ohm at 65 Hz (before voice-coil
  inductance), the largest of the speakers filed so far with complete T/S data.
