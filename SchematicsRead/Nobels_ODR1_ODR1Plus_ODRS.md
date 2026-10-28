# Nobels ODR-1, ODR-1 Plus, ODR-S (read from the three Nobels PDFs and the Andromeda clone document)

Sources: Nobels ODR-1 rev 5 (7 Dec 2000), ODR-1 Plus rev 1 (22 Aug 2003), ODR-S rev 5 (4 Feb 2000), Aion/Andromeda ODR-1 clone PDF (renumbered parts, so its designators differ from Nobels'). Values read from the rasterised schematics; ? marks a value I could not read cleanly. Capacitors in uF unless marked, resistors carbon.

## ODR-1 (the "natural overdrive")

Signal path, in order:

1. Input J1 -> R1 33k -> Q1 BC264D JFET source follower (R2 1M gate bias, R3 3k3 source). Input impedance about 1 M.
2. C1 0.22 -> Q5 K222E JFET used as a series mute switch (D5 4148, R107 1M, C104 47n, gate driven by the XON line). Then C10 0.1, R10 2k7 into U1A (+) input. R11 10k and C11 22n go to the bias rail: with R10 this is a first-order low-pass at about 2.7 kHz on the input to the gain stage.
3. **Gain stage U1A (4558S), non-inverting.** Feedback = Drive pot 250kA in series with R14 1k8, in parallel with C14 120p and two 4148 diodes back to back (D1, D2). The ground leg is two parallel branches: R12 820 + C12 82n, and R13 1k5 + C13 2.2u. Consequences:
   - Mid-band gain at full Drive about 1 + 252k / 1.5k, so about 44 dB; high-frequency gain is higher still (820 and 1k5 in parallel is about 530 ohm above roughly 2.4 kHz), then C14 starts to roll it off above about 5 kHz.
   - Low end: the 1k5 + 2.2u leg corners at about 48 Hz, the 820 + 82n leg at about 2.4 kHz. So gain rises from bass to upper mids: a tight, mid-forward front end. The diodes clip in the feedback loop at about +/-0.6 V, so the clip is soft-ish and the clipping level is fixed while gain varies.
4. **Hard clipper after the gain stage:** C20 2.2 -> R20 12k -> D3, D4 (4148, to the bias rail, anti-parallel) with C21 2n7 across them (corner of 12k with 2.7 n about 4.9 kHz). This is a second, harder shunt clip that follows the soft feedback clip.
5. R21 39k -> R23 10k with C22 82n + R22 12k and C23 1n hanging off the junction (a passive two-section filter in the 39k/10k divider). Then the **Spectrum** network: R24 43k / R25 5k1 divider, R26 1k2, R27 10k, R28 20k, C24 22n, C25 560p around U2A (4558S). Andromeda's description: the Spectrum pot pans between a lowpass at about 700 Hz and a highpass at about 5 kHz, the latter mixed with a fixed upper-mid boost (their text says 2.1 MHz, clearly a typo for roughly 2.1 kHz). Spectrum is 30kB.
6. U2A output -> R32 4k7 -> U2B (4558S) with R33 22k, R34 5k1, R35 1k2, C31 8n2, C32 1u, C33 4n7, C34 82n: a second filter or gain stage that sets the final voicing (more mid hump, then roll-off with C31).
7. C30 2.2 -> Level 50kA -> C41 1u -> Q7 K30A-Y JFET switch (the "effect on" path) -> summing node with R42 330k and C42 1u -> U1B 4558S buffer -> C43 3.3 -> R45 150k -> out.
8. Bypass path: Q1 source -> C2 68n -> Q4 K222E follower -> C40 1u -> Q6 K30A-Y JFET switch (the "effect off" path) -> the same summing node. Footswitch logic is a CD4007 with SW101 (latching), so the bypass is a buffered, electronic switch, not true bypass. A remote footswitch jack J3 (with D101) is on the board.

Supply: 9 V battery or J4, D201 4001, R201/R202 15k divider makes the half-supply bias rail (V/2, called V- in the sheet); C201 220, C202 100, C204 47. LED on 1k5.

## ODR-1 Plus

Identical to the ODR-1 signal path with these differences (all other values match, checked on R10, R12, R13, C11 to C14, R20 to R27, C20 to C34):

- **Boost switch** (SW1B KSA-2240A) adds a third ground leg to the gain stage: R15 270R + C15 2.2u in parallel with R12 and R13. That drops the ground-leg impedance to about 190 ohm (above roughly 270 Hz) and raises gain by about 9 to 10 dB with the boost engaged. So "Boost" is extra gain into the same two clippers, not a clean boost.
- Drive pot P1 250kA, Spectrum P2 30kB, Level P3 50kA with R36 1k in series; output stage the same as the ODR-1 (C43 3.3, R45 150k, U1B buffer).
- Footswitch circuit the same (4007 + SW2204), with jumper headers (JP1, JP2) for the control PCB.

## ODR-S ("Special")

A different circuit with the same skeleton (JFET input buffer, mute JFET, op-amp gain stage, shunt clipper, tone stack, JFET bypass pair, buffered output).

- Input: R1 33k, Q1 BC264D, C1 0.22, Q5 K222E mute switch, C10 0.1, R10 2k7, C13 10n, R11 10k.
- U1A gain stage: Drive 500kA, R14 1k in the feedback (the pot goes through the control PCB), C14 150p; feedback diodes D1, D2 4148; ground legs R12 1k + C11 68n and R13 1k8 + C12 2.2u.
- **After U1A: germanium shunt clipper D3, D4 AA112** (with C21 3n9, R20 10k, C20 3.3). The 4148s in feedback plus germanium shunt diodes give an asymmetric, softer-knee clip than the ODR-1.
- U1B: a second gain stage with a Q2 C1571G transistor (emitter/collector network R24 1k5, R25 200k, R26 3k3, C26 56n, C27 2n7, R21 47k, R22 12k, R23 22k, C23 1u, C24 12n, C25 330p) and a three-way jumper (pins 7, 8, 9) that selects the character of the stage ("Hi" and "Mid" options on the control PCB).
- Tone: U2B and U2A (4558S) form a **three-band EQ**: Hi 50kB, Mid 30kC, Lo 50kB (R30 10k, R31 5k6, R32 2k4, R33 10k, R34 5k6, R35 4k7, R36 10k, R37 15k; C31 0.1, C32 18n, C33 33n, C34 8n2, C35 1n8, C36 22n, C37 1n, C38 82n). It replaces the single Spectrum control.
- Output: C39 1u, Level 50kA, C42 1u, Q7 K30A-Y switch, summing node with R44 330k and C50 1u, **Q4 C1571G emitter follower** (R50 200k, R51 2k2), C51 3.3, R52 200k. Bypass path Q3 K222E -> C41 -> Q6 K30A-Y as in the ODR-1.
- Supply: 4001 reverse protection, R201/R202 10k divider, C201 470, C202 220, R203 100, C203 47.

## What matters for StarterAmp

- A pedal "Natural Overdrive" voicing would be: input low-pass near 2.7 kHz, a very high gain non-inverting stage (up to about 44 dB mid-band, more in the 2 to 5 kHz region) with soft 4148 feedback clipping, then a hard anti-parallel shunt clip, then a 700 Hz to 5 kHz swept "Spectrum" tilt. It is closer to a transparent, mid-forward "preamp overdrive" than to a Tube Screamer: wide Drive range, a lot of upper-mid gain, and two clip stages in series.
- The Andromeda clone's main stated change (a Bass control) is a variable low cut, which confirms the stock ODR-1 low end is not rolled off much (2.2 uF with 1k5 gives only a 48 Hz corner on the gain stage).
- Wired into the code in b64 as the Drive style "Natural Overdrive" (voicings ODR-1, ODR-1 Plus, ODR-S); see CHANGELOG.
