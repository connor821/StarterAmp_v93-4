# Victory Kraken-style preamp pedal: read of a third-party clone schematic (b87)

Source: a PedalPCB build document titled "Cetus Preamp" (rev 12/24/22), supplied by the user as a
Victory Kraken *preamp pedal* schematic. It is a hobbyist PCB project, not the maker's own schematic,
so treat component values as "what the clone uses", not as a verified copy of the original.
It is NOT the Kraken or Super Kraken amp (no schematic exists for those). Supply: 9 V through a
Schottky + 100R, TC1044 charge pump for the negative rail (about +/-8.6 V, estimate). All parts TL072.

Signal path (values as printed; corners and gains are my arithmetic, so estimates):

| Stage | Topology | Corners / gain | Clipping |
|---|---|---|---|
| In | C1 10n, R3 1M to ground, R1 68K | HP about 16 Hz | none |
| IC2.1 | non-inverting. Feedback R4 220K. Gain leg R9 10K + C6 33n | gain rises from x1 to x23 (27 dB) above about 480 Hz | 5V1 + 6V2 zeners in anti-series across R4: about +/-5.8 V and 6.9 V |
| Gain pot | C2 2n2 into A250K | HP about 290 Hz at full pot | none |
| IC1.1 | non-inverting. Feedback R7 100K. Gain leg C7 470n + R11 2K | x51 (34 dB) above about 170 Hz | 2V7 + 3V6 zeners across R7: about 3.4 V and 4.3 V (asymmetric) |
| IC1.2 | inverting. R5 10K in, C3 220n, feedback R2 100K | x10 (20 dB), HP about 72 Hz | 2V7 + 5V1 zeners: about 3.4 V and 5.8 V (asymmetric) |
| Tone stack | R6 220K, C4 470n, C5 220p, R8 43K, C10 33n, C12 10n; Bass A1M with C8 100n and R15 10K; Mid B25K; Treble B250K | passive three-band | none |
| Out | R10 47K + C9 470p (LP about 7 kHz), Volume B1M, IC2.2 non-inverting with R14 220K / R16 33K + C14 100n, C13 220p across R14 | up to x7.7, HP about 48 Hz, gain falls above about 3.3 kHz | 1N4148 pair to the rails |

What it says about the "tight, clear, thick" voice (reading, not measured):
- Every gain leg has a series cap, so gain is rolled off below roughly 170 to 480 Hz *before* it clips.
  The low end is not driven into the clippers, which is what keeps palm-muted low notes tight.
- Three cascaded clippers with different, asymmetric zener thresholds give a progressive, dense saturation
  rather than one hard knee. The first (about 6 V) rarely engages, the second and third (3 to 4 V) do the work.
- The final stage rolls its gain off above about 3.3 kHz and the stack feeds it through a 7 kHz low-pass,
  so the top stays clear without fizz.
- Gain control is *between* stage 1 and stage 2 (a 250K pot after a 2n2 cap), so the pot changes both
  drive and the amount of bass reaching the clippers.

Not usable for: the Kraken amp's own power section, channel switching, or the cab. Only the pedal's preamp voice.
Nothing was built from this read.
