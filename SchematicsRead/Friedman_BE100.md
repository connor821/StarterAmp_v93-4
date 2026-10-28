# Friedman BE-100 (CAD drawing "BE-100 v2", layout by Equinox, drawing by Patrice Frey, May 3rd 2014, REV 1.3)

Source image: `be100-1.png` (3424x886, one very wide CAD sheet; the upper half is readable at crop level, the FX loop and some pot labels are small). This is a third-party redrawn schematic of a BE-100-style amp, not a factory sheet. Values marked (?) are not reliable. Computed values use my own nodal solver of the plugin's tone-stack topology and RC formulas; they are estimates.

## 1. Tube complement
- V1A, V2A, V2B, V3A: ECC83/12AX7 gain stages; V3B: cathode follower (stack driver); V1B and V10 form the clean channel path; V4A/V4B: ECC83 phase inverter; V5, V6, V7, V8: four EL34 power tubes (V5/V7 on one phase, V6/V8 on the other). Eight heaters drawn (V_1 to V_8).
- Solid-state parts: MPSA06 transistor pair (Z1, Z2) used as the SAT clipper; two LND150 depletion-mode MOSFETs (Q1, Q2) in the effects loop; BZX79-C12 zener pair; 7812 regulator for relays; UF4007 power diodes.

## 2. Preamp stage by stage (BE channel, "HBE")

### V1A (first stage)
- Input J1, grid stopper R2 33K, grid leak R1 1M; plate load R9 220K to B+ in series with R3 100K with C8 500 pF across R3 (a plate-load bypass: R3 is shunted at high frequency, so the load falls toward 220K at HF; a plate treble roll-off/boost network depending on the point where the signal is taken); cathode R4 2.7K parallel C5 0.68 uF (corner 1/(2 pi 2.7K 0.68 uF) = 87 Hz).
- Coupling C3 2.2 nF into R5 1M, then R6 68K to ground: a 1M / 68K divider (-23.9 dB at all frequencies, plus a high-pass corner 1/(2 pi (1M + 68K) 2.2 nF) = 68 Hz); then RELAY1 contacts to V2A.

### V2A
- Grid stopper R7 33K, grid leak R11 10M; plate R18 220K in series with R17 100K and C13 500 pF across R17 (same plate network as V1A); cathode R10 2.7K parallel C20 0.68 uF (87 Hz).
- Coupling network after V2A (switchable voicing): C14 2.2 nF directly; C15 22 nF (via SW9 "FAT"); C16 680 pF; series R19 68K or R20 560K (S2 "C45A"); shunt R26 220K or R16 68K (S2/SW8 "C45B"); then R23 33K stopper into V2B.
  - Computed high-pass corners (1/(2 pi (Rseries + Rshunt) C), estimate): 68K + 68K with 2.2 nF about 530 Hz; 560K + 220K with 2.2 nF about 93 Hz; with the FAT 22 nF in parallel (24.2 nF total) 48 Hz and 8.4 Hz. So the network spans roughly 8 Hz to 530 Hz in different positions; the nominal "Tight" (R19 68K / R16 68K, small cap) setting is a 0.5 kHz bass cut, the "Fat" setting a 50 Hz cut. The 560K / 68K and 68K / 220K combinations give intermediate corners; which S2 position pairs with which shunt is (?).
  - 680 pF C16 gives a small extra HF/bright path in parallel with the series resistor.

### V2B (gain control stage)
- Grid R23 33K, gain pot VR1 "Gain 1M" through C19 2.2 nF into R30 470K (with C26 500 pF across) and R28 470K to ground; plate R21 100K; cathode R24 2.7K parallel C17 0.68 uF (87 Hz). RELAY2 and the S7 switch select clean/BE and the Turbo (S7 "TL49PO") function.

### V3A and V3B
- V3A: plate R27 100K with C27 500 pF across it from the B+ line; cathode R29 820R parallel C24 0.68 uF and C43 470 uF/16 V (through SW3 "VOICE", which also selects C42 500 pF at the V3B cathode follower); supply decoupling R34 10K with C28 and C29 22 uF.
- V3B: cathode follower; cathode load R31 100K; R43 10K and C30 100 uF decouple its plate.
- SAT (SW7) adds R33 10K and C25 100 nF/630 V into a pair of MPSA06 transistors (Z1 and Z2) with R38 1M: a transistor-junction clipper hanging off the follower output (soft clipping, a hard-clip pair would conduct about +/-0.6 V). Exact transistor connection (diode-connected antiparallel vs base-emitter breakdown) is (?).

## 3. Tone stack (after V3B)
Printed values: C4 500 pF (treble cap), R8 33K (slope), C1 22 nF/630 V, C6 22 nF/630 V, treble VR6 250kB, bass VR8 1MA, middle VR7 25kB, master VR5 1MA with R59 220K and R32 1M load. Source is the cathode follower (about 1K). This is a JCM800-style TMB stack. Computed with the solver (500 pF, 22 nF, 22 nF, 33K slope; 250K, 1M, 25K; source 1K; load about 1M; bass audio taper): noon mean insertion loss about -6.5 dB, -8.1 dB at 1 kHz, notch about -8.9 dB near 640 Hz; all-10 minimum -7.9 dB near 390 Hz. The JCM800 reference stack (470 pF) gives -6.6 dB mean / -8.3 dB at 1 kHz, so the numbers are nearly identical to a JCM800 / Plexi-style stack with 33K.
- Clean channel: separate stack with R25 100K, C7 0.022 uF, C2 and another 0.022 uF, R42 10K, bass VR1 250kA and treble VR2 1MA, clean volume VR3 and a bright switch SW4 (C10 50 pF, C11 220 pF, C12 100 pF); V1B plate R12 100K, cathode R14 820R; coupling C9 0.022 uF into R15 470K, then V10 (plate R13 100K (?), cathode R14/R20 values (?)). These small clean-channel parts are only partly legible.
- A "Return Lvl" 100kA pot (VR10) and the footswitched effects loop: send J8, return J (TRS), Q1 LND150 (R70 2M2, R60 100K, R61 220K, VR12 50K, C22 470 nF/400 V, C23 10 pF, C50 100 nF/250 V, R71 10R) and Q2 LND150 (R74 110K, R72 1K5, R62 470R, R73 0R, R63 1M, C46 330 nF/250 V, C49 100 nF/400 V, R75 1M), D5 1N4148, D6/D7 BZX79-C12 zeners, C51 22 uF/450 V, R76 33K/2W.

## 4. Phase inverter and power amp
- PI: V4A and V4B (ECC83), long-tail: plates R36 82K and R35 100K, plate-to-plate cap C32 100 pF; shared cathode R39 470R then R40 10K to ground; grid leaks R37 1M and R41 1M to the cathode node side (to the stack/NFB node); input coupling C31 22 nF on V4A, NFB coupling C35 0.1 uF on V4B (feedback is injected on the V4B grid network).
- Coupling C18 22 nF/630 V and C34 22 nF/630 V to the EL34 grids; grid leaks R44 220K and R45 220K (to the bias line), stoppers R66 1.5K (V5/V7) and R67 1.5K (V6/V8).
- Output tubes: four EL34; screen resistors R46, R47, R64, R65 1K/7.5W (printed "1K / 7.5W"); cathodes to ground (fixed bias, the bias trimmer P12 25K with R56 47K, R57 15K, R58 27K, UF4007 D9, C40 and C41 10 uF/100 V forms the bias supply; bias voltage not printed (?)).
- Output transformer T1: IMPEDANCE switch (taps on pins 1 to 4; 4/8/16 ohm positions), speaker jacks J2, J3, Line Out via R49 1.5K and R48 470 / VR11 100K pot (J4).
- NFB and presence: from the output transformer secondary, R50 47K in series with (R51 220K parallel C36 4.7 nF) into the NFB node which is the V4B-side grid/tail node (R40 10K to ground, with C33 0.1 uF/630 V and VR9 "5kB PRESENCE" to ground). Estimates: with the injection node at R40 10K (ideal), the fraction at low frequency is 10K / (10K + 47K + 220K) = 0.036 (about -29 dB); at high frequency C36 4.7 nF (about 6.8K at 5 kHz) shunts R51 so the fraction rises to about 10K / (10K + 47K + 6.4K) = -16 dB (i.e. feedback strengthens at HF, as expected for a "resonant"/damping feedback), and the 5kB presence pot with 0.1 uF shunts the node to ground at HF and so reduces feedback at HF when turned up. Because I cannot confirm the exact node (the printed lines pass behind the PI cathode network), take the -29 dB (LF) to -16 dB (HF) as an estimated range, (?).

## 5. Supply
- Power transformer PT with taps (230 V / 120 V selectable by SW2A/SW2B), four UF4007 diodes (D1 to D4), reservoir two 100 uF/500 V (C37, C45), choke, fuses F1 T4A SLO (mains) and F2 T2A, then C47/C48 33 uF/500 V with R69 100K, R55 100K balance resistors, R68 470K, R54 82K, C38 22 uF/500 V; preamp supply drop R52 10K/2W and R53 4.7K; filament winding 6.3 V.
- Relay supply: RC206 bridge, C39 470 uF/50 V, 7812 regulator, C44 470 uF/16 V, +12 V and 0 V for RELAY1 and RELAY2 (G5V-2-H1, D10 and D11 UF4007 flyback diodes); channel switch SW5 "CLN-BE-HBE" three positions via a TRS footswitch jack J7.
- B+ voltages are not printed on the sheet (?).

## 6. Illegible or uncertain
- The clean channel (V1B, V10) values around the bright switch and the Return pot; exact SW9/C45A/C45B switch positions; B+ and bias voltages; the NFB return node; the FX loop MOSFET stage details; transformer tap voltages.
